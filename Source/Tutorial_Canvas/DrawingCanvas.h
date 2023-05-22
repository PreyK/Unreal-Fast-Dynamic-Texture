// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>
#include "Engine/Texture2D.h"
#include "Object.h"
#include "DrawingCanvas.generated.h"

struct FRenderRegion
{
	bool isDirty;
	FUpdateTextureRegion2D* region;
	FRenderRegion(){}
	FRenderRegion(bool dirty, FUpdateTextureRegion2D* myRegion)
	{
		isDirty = dirty;
		region = myRegion;
	}
}; 

UCLASS(Blueprintable, BlueprintType)
class TUTORIAL_CANVAS_API UDrawingCanvas : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Variables)
	bool UpdateFullRegion = false;
	UTexture2D* dynamicCanvas;
	UFUNCTION(BlueprintCallable, Category = DrawingTools)
		UTexture2D* InitializeCanvas(const int32 pixelsH, const int32 pixelsV,const int32 regionSplit);
	UFUNCTION(BlueprintCallable, Category = DrawingTools)
		void UpdateCanvas();
	UFUNCTION(BlueprintCallable, Category = DrawingTools)
		void ClearCanvas();
	UFUNCTION(BlueprintCallable, Category = DrawingTools)
		void InitializeDrawingTools(const int32 brushRadius);
	UFUNCTION(BlueprintCallable, Category = DrawingTools)
		void DrawDot(const int32 pixelCoordX, const int32 pixelCoordY, int radius);

	UDrawingCanvas();
	~UDrawingCanvas();

private:

	// canvas
	std::unique_ptr<uint8[]> canvasPixelData;
	int canvasWidth;
	int canvasHeight;
	int bytesPerPixel;
	int bufferPitch;
	int bufferSize;

	// draw brush tool
	std::unique_ptr<uint8[]> canvasBrushMask;
	int radius;
	int brushBufferSize;

	// render regions
	int RegionSplits;
	int CellSize;
	int DirtyRegions;
	TArray<TArray<int>> RegionLookup;
	TArray<FRenderRegion> RenderRegions;
	FUpdateTextureRegion2D* FullRegion;
	FUpdateTextureRegion2D* DirtyUpdateRegions;
	
	void setPixelColor(uint8*& pointer, uint8 red, uint8 green, uint8 blue, uint8 alpha);
	bool IsPowerOfTwo(int x);
};