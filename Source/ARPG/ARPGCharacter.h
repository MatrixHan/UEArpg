// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGCharacter.generated.h"

UCLASS(Blueprintable)
class AARPGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AARPGCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UUserWidget> HUDAsset;//ÓÃ»§¿Ø¼þ	

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "3DUI")
	TSubclassOf<AActor> UIMesh3DUI;
private:
	bool isDead;

public:	
	float GetPropertyValue(FString ValueName,bool &IsSuccess);
	float GetPropertyFloatValue(UObject* target ,FString ValueName, bool& IsSuccess);
	bool GetPropertyBoolValue(UObject* target, FString ValueName, bool& IsSuccess);
	bool SetPropertyFloatValue(UObject* target, FString ValueName, float value);
	bool SetPropertyBoolValue(UObject* target, FString ValueName, bool value);
	void UpdateBlood(float value);
	void UpdateManaNum(float value);
	void CheckHealth();
	void SwitchDeadState(bool pIsDead);
	void Relive(float& bloodNum,float& manaNum);
private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

	class AActor* Custom3DUI;

	class UUserWidget* HUD;
};

