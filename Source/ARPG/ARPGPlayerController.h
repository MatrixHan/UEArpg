// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ARPGPlayerController.generated.h"

UCLASS()
class AARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AARPGPlayerController();

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	float blood_num;
	float mana_num;

	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();


	void OnFPressed();
	void OnFReleased();
	void OnDPressed();
	void OnDReleased();
	void OnRPressed();
	void OnRReleased();
public:
	bool GetHitResultUnderCursorByParam(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult, FCollisionQueryParams& Params) const ;
};


