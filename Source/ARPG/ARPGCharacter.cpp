// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPGCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"

#include "ARPGPlayerController.h"
#include "ARPG.h"

#pragma optimize( "", off) 
AARPGCharacter::AARPGCharacter()
{
	
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AARPGCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (CursorToWorld != nullptr)
	{		
		/*TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUIBaseActor::StaticClass(), Actors);*/
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{				
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				//Params.AddIgnoredActors(Actors);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (AARPGPlayerController* PC = Cast<AARPGPlayerController>(GetController()))
		{
			FHitResult TraceHitResult;			
			FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
			//Params.AddIgnoredActor(this);
			//Params.AddIgnoredActors(Actors);
			PC->GetHitResultUnderCursorByParam(ECC_Visibility, true, TraceHitResult, Params);

			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			//UE_LOG(LogARPG, Display, TEXT("Position:Vector(%f,%f,%f)"), CursorFV.X, CursorFV.Y, CursorFV.Z);
			//UE_LOG(LogARPG, Display, TEXT("Rotate(%f,%f,%f)"), CursorR.Vector().X, CursorR.Vector().Y, CursorR.Vector().Z);
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}
}

void AARPGCharacter::BeginPlay()
{
	UWorld* uWord = GetWorld();
	Super::BeginPlay();
	if (UIMesh3DUI) 
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		static FVector OffsetAxisXVector(-65536.0f,0,0);

		Custom3DUI = uWord->SpawnActor<AActor>(UIMesh3DUI, OffsetAxisXVector, FRotator::ZeroRotator, Params);
		//Custom3DUI = uWord->SpawnActor<AActor>(UIMesh3DUI, this->GetActorLocation(), FRotator::ZeroRotator, Params);
	}
	if (HUDAsset)
	{
		HUD = CreateWidget<UUserWidget>(uWord, HUDAsset);
		if (HUD)
		{
			HUD->AddToViewport();
		}
	}

	if (Custom3DUI) 
	{
		FProperty * healthNum = FindFProperty<FProperty>(Custom3DUI->GetClass(), "Health");
		if (healthNum && healthNum->IsA(FFloatProperty::StaticClass()))
		{
			FFloatProperty* FloatProperty = CastField<FFloatProperty>(healthNum);
			float Value = FloatProperty->GetPropertyValue_InContainer(Custom3DUI);
			UE_LOG(LogARPG, Display, TEXT("Bload(%f)"), Value);
			FloatProperty->SetPropertyValue_InContainer(Custom3DUI, 1.0f);
		}	
		FProperty* manaNum = FindFProperty<FProperty>(Custom3DUI->GetClass(), "ManaLiang");
		if (manaNum && manaNum->IsA(FFloatProperty::StaticClass()))
		{
			FFloatProperty* FloatProperty = CastField<FFloatProperty>(manaNum);
			float Value = FloatProperty->GetPropertyValue_InContainer(Custom3DUI);
			UE_LOG(LogARPG, Display, TEXT("Mana(%f)"), Value);
			FloatProperty->SetPropertyValue_InContainer(Custom3DUI, 1.0f);
		}
	}	
}


void AARPGCharacter::UpdateBlood(float value)
{
	if (Custom3DUI) 
	{
		FProperty* healthNum = FindFProperty<FProperty>(Custom3DUI->GetClass(), "Health");
		if (healthNum && healthNum->IsA(FFloatProperty::StaticClass()))
		{
			FFloatProperty* FloatProperty = CastField<FFloatProperty>(healthNum);
			float Value = FloatProperty->GetPropertyValue_InContainer(Custom3DUI);
			UE_LOG(LogARPG, Display, TEXT("Bload(%f)"), Value);
			FloatProperty->SetPropertyValue_InContainer(Custom3DUI, value);
		}
	}
}

void AARPGCharacter::UpdateManaNum(float value)
{
	if (Custom3DUI)
	{
		FProperty* manaNum = FindFProperty<FProperty>(Custom3DUI->GetClass(), "ManaLiang");
		if (manaNum && manaNum->IsA(FFloatProperty::StaticClass()))
		{
			FFloatProperty* FloatProperty = CastField<FFloatProperty>(manaNum);
			float Value = FloatProperty->GetPropertyValue_InContainer(Custom3DUI);
			UE_LOG(LogARPG, Display, TEXT("Mana(%f)"), Value);
			FloatProperty->SetPropertyValue_InContainer(Custom3DUI, value);
		}
	}
}

#pragma optimize( "", on) 