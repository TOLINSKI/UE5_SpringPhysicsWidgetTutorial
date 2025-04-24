// Copyright 2025,  Benski Game Works, All Rights Reserved.

#include "SpringsTutorialWidget.h"
#include "FunctionalUIScreenshotTest.h"
#include "UMG.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "Components/Image.h"

void USpringsTutorialWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Setup variables: (Can be changed in the editor)
	SpringConstant = 200.f;
	DampingCoefficient = 5.f;
	MouseEffect = 0.1f;

	// Setup internal variables:
	SpringThreshold = 0.001f;
	SpringVelocity = 0.f;
	SpringVelocity_Vector = FVector2D::ZeroVector; 
	
	// Getting viewport scale:
	FInt32Vector2 ViewportSizeInt;
	GetOwningPlayer()->GetViewportSize(ViewportSizeInt.X, ViewportSizeInt.Y);
	ViewportScale = GetViewportScaleBasedOnSize(ViewportSizeInt);

	// Store a variable for logo's slot:
	LogoSlot = Cast<UCanvasPanelSlot>(Logo->Slot);

	// Store the starting widget position in widget scale:
	StartPosition_Widget = LogoSlot->GetPosition();
}

FReply USpringsTutorialWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	bMousePressed = true;
	ResetLogo();
	
	return Reply;
}

FReply USpringsTutorialWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

	bMousePressed = false;

	return Reply;
}

void USpringsTutorialWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (bMousePressed)
	{
		AdjustLogo();
		return;
	}
	Spring();
}

void USpringsTutorialWidget::AdjustLogo()
{
	// Get current mouse position (in screen scale)
	FVector2D MousePosition_Screen;
	GetOwningPlayer()->GetMousePosition(MousePosition_Screen.X, MousePosition_Screen.Y);

	// Convert to mouse position in widget scale:
	FVector2D MousePosition_Widget = MousePosition_Screen / ViewportScale;
		
	// Place logo on mouse position if enabled:
	if (bSpringPosition)
	{
		AdjustPosition(MousePosition_Widget);
		return;
	}
		
	// Adjust logo's properties based on mouse X position:
	float AmountToAdjust = MousePosition_Widget.X - StartPosition_Widget.X;

	// Adjust Shear if enabled:
	if (bSpringShear)
	{
		AdjustShear(AmountToAdjust);
		return;		
	}

	// Adjust Scale if enabled:
	if (bSpringScale)
	{
		AdjustScale(AmountToAdjust);
	}
}

void USpringsTutorialWidget::Spring()
{
	if (bSpringShear)
	{
		SpringShear();
		return;
	}
	
	if (bSpringScale)
	{
		SpringScale();
		return;
	}

	if (bSpringPosition)
	{
		SpringPosition();
		return;
	}
}

void USpringsTutorialWidget::AdjustShear(float Amount)
{
	float ShearX = -(Amount * MouseEffect);
	Logo->SetRenderShear(FVector2D(ShearX, 0.0f));
}

void USpringsTutorialWidget::AdjustScale(float Amount)
{
	// Arbitrary Scaler that feels good scale follows the mouse:
	float ScaleByMousePosition =  FMath::Square(MouseEffect) / 10.f;

	// Calculate the scale to set on each axis:
	float ScaleX = 1.f + Amount * ScaleByMousePosition;
	float ScaleY = 2.f - ScaleX;
	Logo->SetRenderScale(FVector2D(ScaleX, ScaleY));
}

void USpringsTutorialWidget::AdjustPosition(FVector2D NewPosition)
{
	// New position is in Widget scale!! 
	LogoSlot->SetPosition(NewPosition);
}

void USpringsTutorialWidget::SpringShear()
{
	float DeltaTime = GetWorld()->GetDeltaSeconds();

	float ShearRestValue = 0.f; // The "spring-shear" rest position is 0, i.e. shear = 0
	float ShearDisplacement = Logo->GetRenderTransform().Shear.X - ShearRestValue; // Shear displacement from rest position

	// Check threshold
	if (ReachedThreshold(ShearDisplacement))
	{
		ResetLogo();
		return;
	}
	
	// Calculate the change in position (velocity):
	float Mass = 1.f; // Mass of the object (for example's sake)

	// Calculate the acceleration the spring would have:
	float SpringForce = - (SpringConstant * ShearDisplacement); // Hooke's law
	float DampingForce = DampingCoefficient * (-SpringVelocity);
	float NetForces = SpringForce + DampingForce;
	float SpringAcceleration = NetForces / Mass; // a = F/m (Newton's second law)
	SpringVelocity += SpringAcceleration * DeltaTime; // Frame-independent velocity from "integrating" acceleration 

	// Calculate the new position:
	float NewShearDisplacement = ShearDisplacement + SpringVelocity * DeltaTime; // Frame-independent displacement from "integrating" velocity
	float NewShearX = ShearRestValue + NewShearDisplacement; 
	Logo->SetRenderShear(FVector2D(NewShearX, 0.0f));
}

void USpringsTutorialWidget::SpringScale()
{
	float DeltaTime = GetWorld()->GetDeltaSeconds();

	float ScaleRestValue = 1.f; // I.e. the default scale = 1
	float ScaleDisplacement = Logo->GetRenderTransform().Scale.X - ScaleRestValue;

	// Check threshold, if the spring is close to rest position and the velocity is low, stop springing:
	if (ReachedThreshold(ScaleDisplacement))
	{
		ResetLogo();
		return;
	}
	
	// Calculate forces:
	float SpringAcceleration = NetForces(ScaleDisplacement); 
	SpringVelocity += SpringAcceleration * DeltaTime;

	// Calculate new scale:
	float NewScaleDisplacement = ScaleDisplacement + SpringVelocity * DeltaTime; // New Displacement
	float NewScaleX = ScaleRestValue + NewScaleDisplacement;  // New Scale X value
	float NewScaleY = FMath::Clamp(2.f - NewScaleX, 0.1f, 1.9f);
	Logo->SetRenderScale(FVector2D(NewScaleX, NewScaleY));
}

void USpringsTutorialWidget::SpringPosition()
{
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FVector2D PositionDisplacement = LogoSlot->GetPosition() - StartPosition_Widget;

	if (ReachedThreshold(PositionDisplacement))
	{
		ResetLogo();
		return;
	}
	
	FVector2D SpringAcceleration = NetForces(PositionDisplacement);
	SpringVelocity_Vector += SpringAcceleration * DeltaTime;

	FVector2D NewPositionDisplacement = PositionDisplacement + SpringVelocity_Vector * DeltaTime;
	FVector2D NewPosition = StartPosition_Widget + NewPositionDisplacement;
	LogoSlot->SetPosition(NewPosition);
}

float USpringsTutorialWidget::NetForces(float Displacement)
{
	// Calculate the acceleration the spring would have:
	float SpringForce = -(SpringConstant * Displacement); // Hooke's law
	float DampingForce = DampingCoefficient * (-SpringVelocity); // Damping force ~Friction
	float NetForces = SpringForce + DampingForce; // Net forces acting on the spring
	
	return NetForces;
}

FVector2D USpringsTutorialWidget::NetForces(FVector2D Displacement_Vector)
{
	FVector2D SpringForce = -(SpringConstant * Displacement_Vector); // Hooke's law
	FVector2D DampingForce = DampingCoefficient * (-SpringVelocity_Vector);
	FVector2D NetForces = SpringForce + DampingForce;

	return NetForces;
}

bool USpringsTutorialWidget::ReachedThreshold(float Displacement)
{
	return FMath::Abs(Displacement) < SpringThreshold && FMath::Abs(SpringVelocity) < SpringThreshold;
}

bool USpringsTutorialWidget::ReachedThreshold(FVector2D Displacement_Vector)
{
	float DisplacementSize = Displacement_Vector.Size();
	float VelocitySize = SpringVelocity_Vector.Size();

	return FMath::Abs(DisplacementSize) < SpringThreshold && FMath::Abs(VelocitySize) < SpringThreshold;
}

float USpringsTutorialWidget::GetViewportScaleBasedOnSize(FInt32Vector2 InViewportSize)
{
	// Need to convert to IntPoint to use with GetDPIScaleBasedOnSize
	FIntPoint PointSize = FIntPoint(InViewportSize.X, InViewportSize.Y);
	return GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(PointSize);
}

void USpringsTutorialWidget::ResetLogo()
{
	// Stop any spring movement:
	SpringVelocity = 0.f;
	SpringVelocity_Vector = FVector2D::ZeroVector;

	// Reset any changed properties:
	Logo->SetRenderShear(FVector2D::ZeroVector);
	Logo->SetRenderScale(FVector2D::UnitVector);
	LogoSlot->SetPosition(StartPosition_Widget);
}