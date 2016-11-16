// Fill out your copyright notice in the Description page of Project Settings.

#include "Platonic.h"
#include "DestroyableObject.h"


// Sets default values
ADestroyableObject::ADestroyableObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADestroyableObject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADestroyableObject::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

