// Copyright 2025,  Benski Game Works, All Rights Reserved.

#include "SpringsTutorialWidget.h"
#include "UMG.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "Components/Image.h"

void USpringsTutorialWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Setup editor variables: (Can be overwritten in the editor)
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
	ViewportSize_Widget = FVector2D(ViewportSizeInt.X, ViewportSizeInt.Y) / ViewportScale;

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
	Spring(); // Physics simulation
}

void USpringsTutorialWidget::AdjustLogo()
{
	// Get current mouse position (in screen scale)
	FVector2D MousePosition_Screen;
	GetOwningPlayer()->GetMousePosition(MousePosition_Screen.X, MousePosition_Screen.Y);

	// Convert to mouse position in widget scale:
	FVector2D MousePosition_Widget = MousePosition_Screen / ViewportScale;
	MousePosition_Widget -= ViewportSize_Widget / 2.f; // Mouse poisiton relative to center

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
	float MousePositionScaler =  FMath::Square(MouseEffect) / 10.f;

	// Calculate the scale to set on each axis:
	float ScaleX = 1.f + Amount * MousePositionScaler; 
	float ScaleY = 2.f - ScaleX;

	// Constraint Scale not to flip:
	ScaleX = FMath::Max(ScaleX, 0.1f);
	ScaleY = FMath::Max(ScaleY, 0.1f);
	
	Logo->SetRenderScale(FVector2D(ScaleX, ScaleY));
}

void USpringsTutorialWidget::AdjustPosition(FVector2D NewPosition_WidgetScale)
{
	LogoSlot->SetPosition(NewPosition_WidgetScale);
}

void USpringsTutorialWidget::SpringShear()
{
	// Current frame variables:
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	float CurrentShearPosition = Logo->GetRenderTransform().Shear.X; // Current shear position

	// Variables for Spring calculation:
	float ShearRestValue = 0.f; // The "spring-shear" rest position is 0, i.e. shear = 0
	float ShearDisplacement = CurrentShearPosition - ShearRestValue; // Shear displacement from rest position
	if (ReachedThreshold(ShearDisplacement))
	{
		ResetLogo();
		return;
	}
	
	// Calculate the rate of change in position (velocity):
	float SpringAcceleration = NetForce(ShearDisplacement);
	SpringVelocity += SpringAcceleration * DeltaTime; // Frame-independent velocity from "integrating" acceleration
	
	// Add the change in position (velocity * delta-time seconds):
	float NewShearPosition = CurrentShearPosition + (SpringVelocity * DeltaTime); // Frame-independent position from "integrating" velocity
	Logo->SetRenderShear(FVector2D(NewShearPosition, 0.0f));
}

void USpringsTutorialWidget::SpringScale()
{
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	float CurrentScalePosition = Logo->GetRenderTransform().Scale.X;
	
	float ScaleRestValue = 1.f; // I.e. the default scale = 1
	float ScaleDisplacement = CurrentScalePosition - ScaleRestValue;
	if (ReachedThreshold(ScaleDisplacement))
	{
		ResetLogo();
		return;
	}

	// Calculate the rate of change in position (velocity):
	float SpringAcceleration = NetForce(ScaleDisplacement); 
	SpringVelocity += SpringAcceleration * DeltaTime;

	// Calculate new position from velocity over delta time:
	float NewScaleX = CurrentScalePosition + (SpringVelocity * DeltaTime);
	float NewScaleY = 2.f - NewScaleX; // Y Decreases as X increases and vice versa

	// Constraint Scale not to flip: Uncomment after exmaple
	NewScaleX = FMath::Max(NewScaleX, 0.1f);
	NewScaleY = FMath::Max(NewScaleY, 0.1f);
	
	Logo->SetRenderScale(FVector2D(NewScaleX, NewScaleY));
}

void USpringsTutorialWidget::SpringPosition()
{
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FVector2D CurrentPosition_Widget = LogoSlot->GetPosition();
	
	FVector2D PositionDisplacement = CurrentPosition_Widget - StartPosition_Widget;
	if (ReachedThreshold(PositionDisplacement))
	{
		ResetLogo();
		return;
	}
	
	FVector2D SpringAcceleration = NetForce(PositionDisplacement);
	SpringVelocity_Vector += SpringAcceleration * DeltaTime;

	FVector2D NewSpringPosition = CurrentPosition_Widget + (SpringVelocity_Vector * DeltaTime);
	LogoSlot->SetPosition(NewSpringPosition);
}

float USpringsTutorialWidget::NetForce(float Displacement)
{
	float SpringForce = -(SpringConstant * Displacement); // Hooke's law
	float DampingForce = DampingCoefficient * (-SpringVelocity); // Damping force ~Friction
	return SpringForce + DampingForce; // Net forces acting on the spring
}

FVector2D USpringsTutorialWidget::NetForce(FVector2D Displacement_Vector)
{
	FVector2D SpringForce = -(SpringConstant * Displacement_Vector); // Hooke's law
	FVector2D DampingForce = DampingCoefficient * (-SpringVelocity_Vector); // Damping force ~Friction
	return SpringForce + DampingForce; // Net forces acting on the spring
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