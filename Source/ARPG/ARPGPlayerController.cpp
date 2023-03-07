// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "ARPGCharacter.h"
#include "Engine/World.h"

AARPGPlayerController::AARPGPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AARPGPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
}

void AARPGPlayerController::SetupInputComponent()
{
	blood_num = 1.0f;
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &AARPGPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &AARPGPlayerController::OnSetDestinationReleased);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AARPGPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AARPGPlayerController::MoveToTouchLocation);

	InputComponent->BindAction("ResetVR", IE_Pressed, this, &AARPGPlayerController::OnResetVR);


	InputComponent->BindAction("FAction",IE_Pressed,this,&AARPGPlayerController::OnFPressed);
	InputComponent->BindAction("FAction", IE_Pressed, this, &AARPGPlayerController::OnFReleased);

	InputComponent->BindAction("DAction", IE_Pressed, this, &AARPGPlayerController::OnDPressed);
	InputComponent->BindAction("DAction", IE_Pressed, this, &AARPGPlayerController::OnDReleased);

}

void AARPGPlayerController::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AARPGPlayerController::MoveToMouseCursor()
{
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (AARPGCharacter* MyPawn = Cast<AARPGCharacter>(GetPawn()))
		{
			if (MyPawn->GetCursorToWorld())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, MyPawn->GetCursorToWorld()->GetComponentLocation());
			}
		}
	}
	else
	{
		// Trace to see what is under the mouse cursor
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			// We hit something, move there
			SetNewMoveDestination(Hit.ImpactPoint);
		}
	}
}

void AARPGPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void AARPGPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if ((Distance > 120.0f))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
		}
	}
}

void AARPGPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void AARPGPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}

void AARPGPlayerController::OnFPressed()
{
	if (AARPGCharacter* MyPawn = Cast<AARPGCharacter>(GetPawn()))
	{
		if (MyPawn)
		{
			blood_num -= 0.1f;
			blood_num = FMath::Clamp(blood_num, 0.0f, 1.0f);
			MyPawn->UpdateBlood(blood_num);
		}
	}
}

void AARPGPlayerController::OnFReleased()
{
}

void AARPGPlayerController::OnDPressed()
{
	if (AARPGCharacter* MyPawn = Cast<AARPGCharacter>(GetPawn()))
	{
		if (MyPawn)
		{
			blood_num += 0.1f;
			blood_num = FMath::Clamp(blood_num,0.0f,1.0f);
			MyPawn->UpdateBlood(blood_num);
		}
	}
}

void AARPGPlayerController::OnDReleased()
{
}

bool AARPGPlayerController::GetHitResultUnderCursorByParam(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult, FCollisionQueryParams& Params) const
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	bool bHit = false;
	if (LocalPlayer && LocalPlayer->ViewportClient)
	{
		FVector2D MousePosition;
		if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
		{
			bHit = GetHitResultAtScreenPosition(MousePosition, TraceChannel,  Params, HitResult);
		}
	}

	if (!bHit)	//If there was no hit we reset the results. This is redundant but helps Blueprint users
	{
		HitResult = FHitResult();
	}

	return bHit;
}
