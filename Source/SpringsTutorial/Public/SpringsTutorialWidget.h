// Copyright 2025,  Benski Game Works, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpringsTutorialWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UImage;

/**
 * 
 */
UCLASS()
class SPRINGSTUTORIAL_API USpringsTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// UWidget variables =============================================================
	/** The object that spreads an XY axis layout on the screen, owns the widgets positioned in the layout. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Springs Tutorial")
	TObjectPtr<UCanvasPanel> CanvasPanel_Root; 

	/** The image to manipulate. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Springs Tutorial")
	TObjectPtr<UImage> Logo;
	
	/** The object connecting the Logo image to the Canvas panel and holds the position/size information. */
	UPROPERTY(BlueprintReadWrite, Category = "Springs Tutorial")
	TObjectPtr<UCanvasPanelSlot> LogoSlot;

	// Override from UUserWidget parent class:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Editable variables ===============================================================
	/** Hooke's law spring constant (Spring Rigidness): */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Springs Tutorial")
	float SpringConstant;

	/** The amount to reduce each step. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Springs Tutorial")
	float DampingCoefficient;	
	
	/** How much should the position of the mouse affect the Logo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Springs Tutorial")
	float MouseEffect;

	UPROPERTY(BlueprintReadWrite, Category = "Springs Tutorial")
	bool bSpringShear = true;

	UPROPERTY(BlueprintReadWrite, Category = "Springs Tutorial")
	bool bSpringScale = false;

	UPROPERTY(BlueprintReadWrite, Category = "Springs Tutorial")
	bool bSpringPosition = false;

	// Internal variables =============================================================
	/** The threshold to stop springing and set values as 0 */
	float SpringThreshold;

	/** The 1D spring velocity. (The change in displacement) */
	UPROPERTY(BlueprintReadWrite, Category = "Springs Tutorial")
	float SpringVelocity;

	/** The 2D vector of SpringVelocity */
	FVector2D SpringVelocity_Vector;
	
	// Functionality variables ========================================================
	bool bMousePressed = false;
	FVector2D StartPosition_Widget;
	float ViewportScale;
	
	// Spring Function =============================================================
	UFUNCTION(BlueprintCallable, Category = "Springs Tutorial")
	void ResetLogo();

	void AdjustLogo();
	
	void Spring();

	void AdjustShear(float Amount);

	void SpringShear();
	
	void AdjustScale(float Amount);

	void SpringScale();

	void AdjustPosition(FVector2D NewPosition);

	void SpringPosition();

	float NetForce(float Displacement);

	FVector2D NetForce(FVector2D Displacement_Vector);

	bool ReachedThreshold(float Displacement);

	bool ReachedThreshold(FVector2D Displacement_Vector);

	float GetViewportScaleBasedOnSize(FInt32Vector2 InViewportSize);
};
