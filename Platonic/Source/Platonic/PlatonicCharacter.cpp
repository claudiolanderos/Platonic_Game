// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Platonic.h"
#include "PlatonicCharacter.h"
#include "CableComponent.h"
#include "Weapon.h"
#include "Engine.h"
//#include "Blueprint/UserWidget.h"
#include <cmath>        // std::abs

APlatonicCharacter::APlatonicCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Rotation of the character should not affect rotation of boom
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 750.f;
	CameraBoom->SocketOffset = FVector(0.f,0.f,75.f);
	CameraBoom->RelativeRotation = FRotator(0.f,180.f,0.f);

	// Create a camera and attach to boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideViewCameraComponent->bUsePawnControlRotation = false; // We don't want the controller rotating the camera

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Face in the direction we are moving..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;
//    GetCharacterMovement()->bConstrainToPlane = true;
//    GetCharacterMovement()->SetPlaneConstraintFromVectors(GetActorForwardVector(), GetActorUp)
    // Initialise the can crouch property
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
    
    // Add GrappleLine
    GrappleLine = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleLine"));
    GrappleLine->SetWorldLocation(this->GetActorLocation());
    GrappleLine->SetVisibility(false);

    MyWeapon = nullptr;
}

void APlatonicCharacter::BeginPlay() {
    Super::BeginPlay();
    
    if (Weapon) {
        UWorld* World = GetWorld();
        if (World) {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;
            SpawnParams.Instigator = Instigator; // Need to set rotation like this because otherwise gun points down
            FRotator Rotation(0.0f, 0.0f, -90.0f);
            // Spawn the Weapon
            MyWeapon = World->SpawnActor<AWeapon>(Weapon, FVector::ZeroVector, Rotation, SpawnParams);
            if (MyWeapon) { // This is attached to "WeaponPoint" which is defined in the skeleton
                MyWeapon->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepRelativeTransform, TEXT("WeaponPoint"));
            }
        }
    }
    
    APlayerController* MyController = GetWorld()->GetFirstPlayerController();
    
    if (MyController)
    {
        MyController->bShowMouseCursor = true;
        MyController->bEnableClickEvents = true;
        MyController->bEnableMouseOverEvents = true;
    }
    
    UWorld* World = GetWorld();
    if (World)
    {
        if (GrappleLine)
        {
//            GrappleLine->SetVisibility(false);
            GrappleLine->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("WeaponPoint"));
            GrappleLine->CableLength = 0;
            GrappleLine->CableWidth = 6;
//            GrappleLine->
        }
        
        vCameraPos = CameraBoom->GetComponentLocation();
        FVector vPlayerUp = this->GetActorUpVector();
        vPlayerUp.Normalize();
        vCameraUp = CameraBoom->GetUpVector();
        vCameraUp.Normalize();
        FVector vCameraForward = CameraBoom->GetForwardVector();
        vCameraForward.Normalize();
        vCameraLeft = FVector::CrossProduct(vPlayerUp, vCameraForward);
        vCameraLeft.Normalize();
        cameraSpeed = 100;
    }
    
    APlayerController* playerController = GetWorld()->GetFirstPlayerController();
    hud = playerController->GetHUD();
//    hud->DrawHUD();
    
//    FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
//    hud->DrawText(CurrentLevel, FLinearColor::White, ViewportSize.X/2, ViewportSize.Y/2);
//    hud->ShowHUD();
    
}


// Input

void APlatonicCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APlatonicCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlatonicCharacter::MoveRight);
    PlayerInputComponent->BindAction("Grapple", IE_Pressed, this, &APlatonicCharacter::Grapple);
    PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &APlatonicCharacter::StartCrouch);
    PlayerInputComponent->BindAction("Crouch", IE_Released, this, &APlatonicCharacter::StopCrouch);
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &APlatonicCharacter::BeginSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &APlatonicCharacter::EndSprint);
	PlayerInputComponent->BindTouch(IE_Pressed, this, &APlatonicCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &APlatonicCharacter::TouchStopped);
}

void APlatonicCharacter::Jump() {
    if (Hooked) {
        LaunchCharacter(FVector(0,0,750), false, false);
        StopGrapple();
        Super::Jump();
    } else {
        StopGrapple();
        Super::Jump();
    }
}

void APlatonicCharacter::StartCrouch()
{
    Crouch();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Crouching"));

}

void APlatonicCharacter::StopCrouch()
{
    UnCrouch();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Stop Crouching"));
}

void APlatonicCharacter::BeginSprint()
{
    if(!bIsCrouched)
    {
        bIsSprinting = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Sprinting"));
    }
}

void APlatonicCharacter::EndSprint()
{
    bIsSprinting = false;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Stop Sprinting"));

}

void APlatonicCharacter::MoveRight(float Value)
{
	// add movement in that direction
    if(bIsSprinting)
    {
        Value *= 2;
    }
	AddMovementInput(FVector(0.f,-1.f,0.f), Value / 2);
}

void APlatonicCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// jump on any touch
	Jump();
}

void APlatonicCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}

void APlatonicCharacter::Grapple()
{
    // Start from the muzzle's position
    FVector StartPos = this->GetActorLocation();
    APlayerController* playerController = GetWorld()->GetFirstPlayerController();
    float x,y;
    
    if (!playerController->GetMousePosition(x, y))
    {
        UE_LOG(LogTemp, Log, TEXT("Failed"));
    } else {
        UE_LOG(LogTemp, Log, TEXT("Succeded"));
    }
    FVector2D MousePosition(x, y);
    FHitResult HitResult;
    const bool bTraceComplex = false;
    if (playerController->GetHitResultAtScreenPosition(MousePosition, ECC_Visibility, bTraceComplex, HitResult) == true )
    {
        // If the actor we intersected with is a controller posses it
        if (HitResult.bBlockingHit && std::abs(HitResult.Location.X - StartPos.X) < 50.0f)
        {
            Hooked = true;
            hookLocation = HitResult.Location;
        }
        else
        {
            Hooked = false;
            HookMoveFinished = false;
            StopGrapple();
        }
    }
}

void APlatonicCharacter::Tick(float deltaTime) {
    if (Hooked) {
        if (HookMoveFinished) {
            if (!playerMoveFinished) {
                MoveGrappledPlayer();
            }
            else {
                StopGrapple();
            }
        } else {
            HookMoveFinished = MoveRope();
        }
    }
    
    // camera tracks player's vertical
    float vertical = CameraBoom->GetComponentLocation().Z - vCameraPos.Z;
    
    FVector PlayerPos = this->GetActorLocation();
    
    // check if surpassed left-boundscam
    if (vCameraPos.Y + 650 < PlayerPos.Y) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("STARTING OVER"));
        FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
//        hud->DrawText("STARTING OVER", FLinearColor::White, ViewportSize.X/2, ViewportSize.Y/2);
//        hud->ShowHUD();
        UGameplayStatics::OpenLevel(GetWorld(),FName(*UGameplayStatics::GetCurrentLevelName(GetWorld())));
    }
    
    // check if surpassed right-bounds (2/3 from the left side of screen)
    if (vCameraPos.Y - 300 > PlayerPos.Y) {
//        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PASSED RIGHT"));
        vCameraPos += vCameraLeft * (vCameraPos.Y - 300 - PlayerPos.Y) + (FVector(0.f, 0.f, vertical));
    } else {
        vCameraPos += vCameraLeft * cameraSpeed * deltaTime + (FVector(0.f, 0.f, vertical));
    }

    CameraBoom->SetWorldLocation(vCameraPos);
}

bool APlatonicCharacter::MoveRope() {
    GrappleLine->SetVisibility(true);
    // Get Location
    FVector loc = GrappleLine->GetComponentLocation();
    
    FVector result = hookLocation - loc;
    bool reachedLocation;
    if (result.Size() <= 100.0f) {
        return true;
    } else {
        auto interpLoc = FMath::VInterpTo(GrappleLine->GetComponentLocation(), hookLocation, this->GetWorld()->GetTimeSeconds(), 0.1);
        GrappleLine->SetWorldLocation(interpLoc);
        return false;
    }
}

void APlatonicCharacter::MoveGrappledPlayer() {
    auto loc = hookLocation - this->GetActorLocation();
    loc.Normalize();
    loc *= 2500;
    LaunchCharacter(loc, true, true);
    if ((hookLocation - this->GetActorLocation()).Size() < 200.0f) {
        playerMoveFinished = true;
    }
}

void APlatonicCharacter::StopGrapple() {
    Hooked = false;
    HookMoveFinished = false;
    playerMoveFinished = false;
    GrappleLine->SetVisibility(false);
    GrappleLine->SetWorldLocation(this->GetActorLocation());
}
