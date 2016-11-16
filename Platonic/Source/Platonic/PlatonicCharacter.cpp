// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Platonic.h"
#include "PlatonicCharacter.h"
#include "CableComponent.h"

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
	CameraBoom->TargetArmLength = 500.f;
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
    // Initialise the can crouch property
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
    
    // Add GrappleLine
    GrappleLine = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleLine"));
}

void APlatonicCharacter::BeginPlay() {
   
    Super::BeginPlay();
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
//            GrappleLine->EndLocation = FVector::ZeroVector;
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
        cameraSpeed = 5;
    }
}

//////////////////////////////////////////////////////////////////////////
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
    if(CanCrouch() == true)
    {
        Crouch();
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Crouching"));
    }
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
    static FName WeaponFireTag = FName(TEXT("WeaponTrace"));
    static FName MuzzleSocket = FName(TEXT("MuzzleFlashSocket"));
    // Start from the muzzle's position
    FVector StartPos = this->GetActorLocation();
    // Get forward vector of MyPawn
    FVector Forward = GetActorForwardVector();
    // Calculate end position
    FVector EndPos = Forward * GrappleRange + StartPos;
    
    // Perform trace to retrieve hit info
    FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
    TraceParams.bTraceAsyncScene = true;
    TraceParams.bReturnPhysicalMaterial = true;
    
    // This fires the ray and checks against all objects w/ collision
    FHitResult Hit(ForceInit);
    GetWorld()->LineTraceSingleByObjectType(Hit, StartPos, EndPos, FCollisionObjectQueryParams::AllObjects, TraceParams);
    
    FVector loc = FVector::ZeroVector;
    
    // Did this hit anything?
    if (Hit.bBlockingHit)
    {
        Hooked = true;
        hookLocation = Hit.Location;
//        GrappleLine->EndLocation = hookLocation;
    }
    else
    {
        Hooked = false;
        StopGrapple();
    }
}

void APlatonicCharacter::Tick(float deltaTime) {
    if (Hooked) {
        if (HookMoveFinished) {
            MoveGrappledPlayer();
            StopGrapple();
        } else {
            HookMoveFinished = MoveRope();
        }
    }
    
    float vertical = CameraBoom->GetComponentLocation().Z - vCameraPos.Z;
    
    vCameraPos += vCameraLeft * cameraSpeed + (FVector(0.f, 0.f, vertical));
    CameraBoom->SetWorldLocation(vCameraPos);
}

bool APlatonicCharacter::MoveRope() {
    GrappleLine->SetVisibility(true);
    // Get Location
    FVector loc = GrappleLine->GetComponentLocation();
    
    FVector result = hookLocation - loc;
    bool reachedLocation;
    if (result.Size() <= 100.0f) {
        reachedLocation = true;
    } else {
        auto interpLoc = FMath::VInterpTo(GrappleLine->GetComponentLocation(), hookLocation, this->GetWorld()->GetTimeSeconds(), 0.1);
        GrappleLine->SetWorldLocation(interpLoc);
        reachedLocation = false;
    }
    
    return reachedLocation;
}

void APlatonicCharacter::MoveGrappledPlayer() {
    auto loc = hookLocation - this->GetActorLocation();
    loc *= this->GetWorld()->GetTimeSeconds() * grappleSpeed;
    LaunchCharacter(loc, true, true);
}

void APlatonicCharacter::StopGrapple() {
    Hooked = false;
    HookMoveFinished = false;
//    GrappleLine->SetVisibility(false);
//    GrappleLine->EndLocation = FVector::ZeroVector;
    GrappleLine->SetWorldLocation(this->GetActorLocation());
}
