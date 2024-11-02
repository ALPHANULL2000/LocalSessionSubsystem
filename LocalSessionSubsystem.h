// ALPHANULL2000 - November 01 2024

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "LocalSessionSubsystem.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LocalSessionSubsystem, Log, All);

struct FOnlineSessionSetting;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionDestroyed, const FString&, SessionName, const bool, Succeed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FOnSessionFound, const bool, Succeed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionCreated, const FString&, SessionName, const bool, Succeed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSessionJoined, const FString&, SessionName, const bool, Succeed,
	const uint8, JoinResultCode);

/**
 * This class has some helper finctions for creating and
 * managing (Join / Create) LAN sessions.
 * You can call it directly in Editor and use its functions.
 */
UCLASS()
class ALPHANULLSESSIONS_API ULocalSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

	ULocalSessionSubsystem();

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:

	IOnlineSessionPtr SessionInterface;
	FName DesiredSessionName;
	FString DesiredMapName;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FName FindingSessionName;
	int32 CreatingSessionNetID = 0;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaximumPawnsPerSession = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoDestroyDublicatedNamedSessions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerMapsFolder = FString("/Game/AlphanullSessions/LAN/Levels");

private:

	const FString MakeLANAddress(const bool Listen = true) const;

public:

	UFUNCTION(BlueprintCallable)
	void SetFindingSessionName(const FName& NewFindName);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FName GetFindingSessionName() const;



	UFUNCTION(BlueprintCallable)
	void SetDesiredSessionName(const FName& NewDesiredName);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FName GetDesiredSessionName() const;



	UFUNCTION(BlueprintCallable)
	void SetServerMapsFolder(const FString& Folder);

	UFUNCTION(BlueprintCallable)
	void ResetServerMapsFolder(FString& OutDefaultFolder);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FString GetServerMapsFolder() const;

public:

	UFUNCTION(BlueprintCallable)
	void CreateSession(const FString& CreatingName, const FString& ListenMap);

	UFUNCTION(BlueprintCallable)
	void FindSession(const FString& FindingName);

protected:

	void OnSessionCreatedCallBack(FName SessionName, bool Succeed);
	void OnJoinedSessionCallBack(FName SessionName, EOnJoinSessionCompleteResult::Type ResultType);
	void OnSessionDestroyedCallBack(FName SessionName, bool Succeed);
	void OnSessionFoundCallBack(bool Succeed);

protected:

	UPROPERTY(BlueprintAssignable)
	FOnSessionDestroyed OnSessionDestroyed;

	UPROPERTY(BlueprintAssignable)
	FOnSessionFound OnSessionFound;

	UPROPERTY(BlueprintAssignable)
	FOnSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable)
	FOnSessionJoined OnSessionJoined;
};
