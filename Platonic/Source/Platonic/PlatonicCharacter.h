// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"
#include "PlatonicCharacter.generated.h"

UCLASS(config=Game)
class APlatonicCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	/** Camera boom positioning the camera beside the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
    
    UPROPERTY(Category = Cable, EditAnywhere) class UCableComponent* GrappleLine;
    
    UPROPERTY(EditAnywhere) class UPhysicsConstraintComponent* PhysicsComponent;
    
    
    AHUD* hud;
    
    float GrappleRange = 2000.0f;
    
    FVector hookLocation = FVector::ZeroVector;
    
    float grappleSpeed = 0.8f;
    bool Hooked = false;
    bool HookMoveFinished = false;
    bool playerMoveFinished = false;
    
    void MoveGrappledPlayer();
    bool MoveRope();
    void StopGrapple();
    
    FVector vCameraPos;
    FVector vCameraLeft;
    FVector vCameraUp;
    float cameraSpeed;
    class AWeapon* MyWeapon;

protected:

    UPROPERTY(EditAnywhere, Category = Weapon) TSubclassOf<class AWeapon> Weapon;
    
	/** Called for side to side input */
	void MoveRight(float Val);
    
    void Tick(float deltaTime) override;

	/** Handle touch inputs. */
	void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Handle touch stop event. */
	void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface


public:
	APlatonicCharacter();
    
    void BeginPlay() override;
    
    void Grapple();
    
    void Jump() override;

    void StartCrouch();
    
    void StopCrouch();
    
    void BeginSprint();
    
    void EndSprint();
    
    bool bIsSprinting = false;
    
    
	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    
    FORCEINLINE class UCableComponent* GetGrappleLine() const { return GrappleLine; }
    
};
