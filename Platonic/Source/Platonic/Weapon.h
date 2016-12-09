// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UCLASS()
class PLATONIC_API AWeapon : public AActor
{
	GENERATED_BODY()
    
    
protected:
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Weapon) USkeletalMeshComponent* WeaponMesh;
    
public:	
	// Sets default values for this actor's properties
	AWeapon();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;
    
    USkeletalMeshComponent* GetWeaponMesh() {
        return WeaponMesh;
    }

};
