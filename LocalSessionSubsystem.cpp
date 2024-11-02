// ALPHANULL2000 - November 01 2024

#include "LocalSessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"



ULocalSessionSubsystem::ULocalSessionSubsystem()
{
	// Construction...
}

void ULocalSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LocalSessionSubsystem, Log, 
		TEXT("ULocalSessionSubsystem::Initialize : { Initialized. }"));

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LocalSessionSubsystem, Error,
			TEXT("ULocalSessionSubsystem::Initialize : { SessionInterface instance not valid, Deinitializing. }"));
		Deinitialize();
		return;
	}

	SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &ULocalSessionSubsystem::OnSessionCreatedCallBack);
	SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &ULocalSessionSubsystem::OnJoinedSessionCallBack);
	SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &ULocalSessionSubsystem::OnSessionDestroyedCallBack);
	SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &ULocalSessionSubsystem::OnSessionFoundCallBack);
}

void ULocalSessionSubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LocalSessionSubsystem, Log,
		TEXT("ULocalSessionSubsystem::Deinitialize : { De-Initialized. }"));
}



const FString ULocalSessionSubsystem::MakeLANAddress(const bool Listen) const
{
	FString FinalAddress = ServerMapsFolder.EndsWith(FString("/")) ? ServerMapsFolder : (ServerMapsFolder + FString("/"));
	FinalAddress.Append(DesiredMapName);
	if (Listen)
	{
		FinalAddress.Append(FString("?listen"));
	}
	return FinalAddress;
}



void ULocalSessionSubsystem::SetFindingSessionName(const FName& NewFindName)
{
	if (NewFindName.IsNone())
	{
		UE_LOG(LocalSessionSubsystem, Warning, 
			TEXT("ULocalSessionSubsystem::SetFindingSessionName : { Session name cannot be empty. }"));
		return;
	}
	FindingSessionName = NewFindName;
}

const FName ULocalSessionSubsystem::GetFindingSessionName() const
{
	return FindingSessionName;
}

void ULocalSessionSubsystem::SetDesiredSessionName(const FName& NewDesiredName)
{
	if (NewDesiredName.IsNone())
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::SetDesiredSessionName : { Session name cannot be empty. }"));
		return;
	}
	DesiredSessionName = NewDesiredName;
}

const FName ULocalSessionSubsystem::GetDesiredSessionName() const
{
	return DesiredSessionName;
}



void ULocalSessionSubsystem::SetServerMapsFolder(const FString& Folder)
{
	if (Folder.IsEmpty())
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::SetServerMapsFolder : { Folder address cannot be empty. }"));
		return;
	}

	ServerMapsFolder = Folder;
}

void ULocalSessionSubsystem::ResetServerMapsFolder(FString& OutDefaultFolder)
{
	ServerMapsFolder = FString("/Game/AlphanullSessions/LAN/Levels");
	UE_LOG(LocalSessionSubsystem, Log, 
		TEXT("ULocalSessionSubsystem::ResetServerMapsFolder : { ServerMapsFolder path reset successfully. }"));
}

const FString ULocalSessionSubsystem::GetServerMapsFolder() const
{
	return ServerMapsFolder;
}



void ULocalSessionSubsystem::CreateSession(const FString& CreatingName, const FString& ListenMap)
{
	if (CreatingName.IsEmpty())
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::CreateSession : { Creating sesion name cannot be empty, task ignored. }"));
		return;
	}

	check(SessionInterface);
	SetDesiredSessionName(FName(*CreatingName));
	
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.NumPublicConnections = MaximumPawnsPerSession;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL") ? true : false;
	SessionSettings.Set(FName("SERVER_NAME"), CreatingName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	FNamedOnlineSession* NamedOnline = SessionInterface->GetNamedSession(FName(*CreatingName));
	if (NamedOnline)
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::CreateSession : { Duplicated session name detected, %s. }"),
			(bAutoDestroyDublicatedNamedSessions ? *FString("Destroying old one") : *FString("Task ignored")));

		if (!bAutoDestroyDublicatedNamedSessions)
		{
			return;
		}
		SessionInterface->DestroySession(FName(*CreatingName));
	
	}
	SessionInterface->CreateSession(CreatingSessionNetID++, FName(*CreatingName), SessionSettings);
	DesiredMapName = ListenMap;
}

void ULocalSessionSubsystem::FindSession(const FString& FindingName)
{
	if (FindingName.IsEmpty())
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::FindSession : { Finding sesion name cannot be empty, task ignored. }"));
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 1024;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Type::Equals);
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	FindingSessionName = FName(*FindingName);
}



void ULocalSessionSubsystem::OnSessionCreatedCallBack(FName SessionName, bool Succeed)
{
	if (!Succeed)
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnSessionCreatedCallBack : { Session did not created: %s. }"), *SessionName.ToString());
		OnSessionCreated.Broadcast(SessionName.ToString(), Succeed);
		return;
	}

	UE_LOG(LocalSessionSubsystem, Log,
		TEXT("ULocalSessionSubsystem::OnSessionCreatedCallBack : { Session created: %s . }"), *SessionName.ToString());
	
	check(GetWorld());
	GetWorld()->ServerTravel(MakeLANAddress(true), true, false);
	DesiredSessionName = SessionName;

	OnSessionCreated.Broadcast(SessionName.ToString(), Succeed);
}

void ULocalSessionSubsystem::OnJoinedSessionCallBack(FName SessionName, EOnJoinSessionCompleteResult::Type ResultType)
{
	if (ResultType != EOnJoinSessionCompleteResult::Type::Success)
	{
		switch (ResultType)
		{
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			UE_LOG(LocalSessionSubsystem, Warning,
				TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Failed, AlreadyInSession : %s. }"), *SessionName.ToString());
			break;

		case EOnJoinSessionCompleteResult::SessionIsFull:
			UE_LOG(LocalSessionSubsystem, Warning,
				TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Failed, SessionFull : %s. }"), *SessionName.ToString());
			break;

		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			UE_LOG(LocalSessionSubsystem, Warning,
				TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Failed, NoRetrievedAddress : %s. }"), *SessionName.ToString());
			break;

		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			UE_LOG(LocalSessionSubsystem, Warning,
				TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Failed, NotExists : %s. }"), *SessionName.ToString());
			break;

		case EOnJoinSessionCompleteResult::UnknownError:
			UE_LOG(LocalSessionSubsystem, Warning,
				TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Failed, Unknown : %s. }"), *SessionName.ToString());
			break;
		}
		OnSessionJoined.Broadcast(SessionName.ToString(), false, ResultType);
		return;
	}

	check(SessionInterface);
	FString ResolvedAddress;
	if (!SessionInterface->GetResolvedConnectString(DesiredSessionName, ResolvedAddress, NAME_GamePort))
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Cannot resolve connection/address, Task ignored. }"));
		OnSessionJoined.Broadcast(SessionName.ToString(), false, ResultType);
		return;
	}

	UE_LOG(LocalSessionSubsystem, Warning,
		TEXT("ULocalSessionSubsystem::OnJoinedSessionCallBack : { Joined successfully to session : %s. }"), *SessionName.ToString());

	APlayerController* PlayerCtrl = GetGameInstance()->GetFirstLocalPlayerController();
	check(PlayerCtrl);
	PlayerCtrl->ClientTravel(ResolvedAddress, ETravelType::TRAVEL_Absolute);

	OnSessionJoined.Broadcast(SessionName.ToString(), true, ResultType);
}

void ULocalSessionSubsystem::OnSessionDestroyedCallBack(FName SessionName, bool Succeed)
{
	if (!Succeed)
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnSessionDestroyedCallBack : { Failed to destroy session: %s . }"), *SessionName.ToString());
		OnSessionDestroyed.Broadcast(SessionName.ToString(), false);
		return;
	}

	UE_LOG(LocalSessionSubsystem, Log,
		TEXT("ULocalSessionSubsystem::OnSessionDestroyedCallBack : { Session destroyed: %s . }"), *SessionName.ToString());

	OnSessionDestroyed.Broadcast(SessionName.ToString(), true);
}

void ULocalSessionSubsystem::OnSessionFoundCallBack(bool Succeed)
{
	if (!Succeed)
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { Session not found: $s . }"), *FindingSessionName.ToString());
		OnSessionFound.Broadcast(false);
		return;
	}
	if (FindingSessionName.IsNone())
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { Session name is none, Task ignored. }"));
		OnSessionFound.Broadcast(false);
		return;
	}
	if (!SessionSearch)
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { Session search is inValid, Task ignored. }"));
		OnSessionFound.Broadcast(false);
		return;
	}

	TArray<FOnlineSessionSearchResult> SearchResults = SessionSearch->SearchResults;
	FOnlineSessionSearchResult CorrectResult;
	
	if (SearchResults.IsEmpty())
	{
		UE_LOG(LocalSessionSubsystem, Warning,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { No sessions found, Task ignored. }"));
		OnSessionFound.Broadcast(false);
		return;
	}

	UE_LOG(LocalSessionSubsystem, Log,
		TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { Number of found sessions: %d . }"), SearchResults.Num());

	FString TempResultName;
	for (FOnlineSessionSearchResult Result : SearchResults)
	{
		if (!Result.IsValid())
		{
			continue;
		}
		Result.Session.SessionSettings.Get(FName("SERVER_NAME"), TempResultName);
		UE_LOG(LocalSessionSubsystem, Log,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { Named session found: %s . }"), *TempResultName);

		if (TempResultName.Equals((FindingSessionName.ToString()), ESearchCase::Type::CaseSensitive))
		{
			CorrectResult = Result;
			DesiredSessionName = FName(TempResultName);
			UE_LOG(LocalSessionSubsystem, Log,
				TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { Session found and joinning: %s. }"), *TempResultName);
			break;
		}
	}

	if (!CorrectResult.IsValid())
	{
		UE_LOG(LocalSessionSubsystem, Error,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { No session found. }"));
		OnSessionFound.Broadcast(false);
		return;
	}
	if (DesiredSessionName.IsNone())
	{
		UE_LOG(LocalSessionSubsystem, Error,
			TEXT("ULocalSessionSubsystem::OnSessionFoundCallBack : { DesiredSessionName did't set fine. }"));
		OnSessionFound.Broadcast(false);
		return;
	}

	check(SessionInterface);
	SessionInterface->JoinSession(CreatingSessionNetID, DesiredSessionName, CorrectResult);
	OnSessionFound.Broadcast(true);
}
