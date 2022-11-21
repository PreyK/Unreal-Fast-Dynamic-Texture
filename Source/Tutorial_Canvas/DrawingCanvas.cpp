// Fill out your copyright notice in the Description page of Project Settings.

#include "DrawingCanvas.h"
UDrawingCanvas::UDrawingCanvas(){}
UDrawingCanvas::~UDrawingCanvas(){}

//region split splits the whole texture into n regions for faster updates
UTexture2D* UDrawingCanvas::InitializeCanvas(const int32 pixelsH, const int32 pixelsV, const int32 regionSplit = 4)
{
	//Make sure we only ever split the canvas to power of 2
	if(IsPowerOfTwo(regionSplit))
	{
		RegionSplits = regionSplit;
	}
	else
	{
		RegionSplits = 4;
		FString logtext = FString("Region split ")+FString::FromInt(regionSplit)+FString(" is not Power Of 2!, defaulting to 4 splits");
		if(GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, logtext);

		UE_LOG(LogTemp, Error, TEXT("%s"), *logtext);
	}
	//dynamic texture initialization
	canvasWidth = pixelsH;
	canvasHeight = pixelsV;
	dynamicCanvas = UTexture2D::CreateTransient(canvasWidth, canvasHeight);
#if WITH_EDITORONLY_DATA
	dynamicCanvas->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif
	dynamicCanvas->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	dynamicCanvas->SRGB = 1;
	dynamicCanvas->AddToRoot();
	dynamicCanvas->Filter = TextureFilter::TF_Nearest;
	dynamicCanvas->UpdateResource();
	
	//full region for clear and performance comparison
	FullRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, canvasWidth, canvasHeight);
	
	//region lookup array[pixelX][pixelY] initialization
	RegionLookup.SetNum(RegionSplits);
	for (int i = 0; i < RegionSplits; i++)
	{
		RegionLookup[i].SetNum(RegionSplits);
	}
	
	CellSize = canvasWidth/RegionSplits;
	//render region initialization
	for (int y = 0; y < RegionSplits; y++)
	{
		for (int x = 0; x < RegionSplits; x++)
		{
			FRenderRegion region = *new FRenderRegion(false, new FUpdateTextureRegion2D(
				canvasWidth/RegionSplits*x,
				canvasHeight/RegionSplits*y,
				canvasWidth/RegionSplits*x,
				canvasHeight/RegionSplits*y,
				canvasWidth/RegionSplits,
				canvasHeight/RegionSplits));
			RenderRegions.Add(region);
			//we can query a any position's render region via RegionLookup[pixelX][pixelY]
			RegionLookup[region.region->DestX/CellSize][region.region->DestY/CellSize] = RenderRegions.Num()-1;
		}
	}
	//we will pass everything that's dirty to this, this will be rendered
	DirtyUpdateRegions = new FUpdateTextureRegion2D[RenderRegions.Num()];
	
	// buffers initialization
	bytesPerPixel = 4; // r g b a
	bufferPitch = canvasWidth * bytesPerPixel;
	bufferSize = canvasWidth * canvasHeight * bytesPerPixel;
	canvasPixelData = std::unique_ptr<uint8[]>(new uint8[bufferSize]);
	
	ClearCanvas();
	return dynamicCanvas;
}

void UDrawingCanvas::InitializeDrawingTools(const int32 brushRadius)
{
	radius = brushRadius;
	brushBufferSize = radius * radius * 4 * bytesPerPixel; //2r*2r * bpp
	canvasBrushMask = std::unique_ptr<uint8[]>(new uint8[brushBufferSize]);
	uint8* canvasBrushPixelPtr = canvasBrushMask.get();
	for (int px = -radius; px < radius; ++px)
	{
		for (int py = -radius; py < radius; ++py)
		{
			int32 tx = px + radius;
			int32 ty = py + radius;
			canvasBrushPixelPtr = canvasBrushMask.get() + (tx + + ty * 2 * radius) * bytesPerPixel;
			if (px * px + py * py < radius * radius)
			{
				setPixelColor(canvasBrushPixelPtr, 0, 0, 0, 255); //black alpha 255 - bgra
			}
			else
			{
				setPixelColor(canvasBrushPixelPtr, 0, 0, 0, 0); // alpha 0
			}
		}
	}
}
void UDrawingCanvas::DrawDot(const int32 pixelCoordX, const int32 pixelCoordY)
{
	uint8* canvasPixelPtr = canvasPixelData.get();
	const uint8* canvasBrushPixelPtr = canvasBrushMask.get();
	for (int px = -radius; px < radius; ++px)
	{
		for (int py = -radius; py < radius; ++py)
		{
			int32 tbx = px + radius;
			int32 tby = py + radius;
			canvasBrushPixelPtr = canvasBrushMask.get() + (tbx + tby * 2 * radius) * bytesPerPixel;
			if (*(canvasBrushPixelPtr + 3) == 255) // check the alpha value of the pixel of the brush mask
			{
				int32 tx = pixelCoordX + px;
				int32 ty = pixelCoordY + py;
				if (tx >= 0 && tx < canvasWidth && ty >= 0 && ty < canvasHeight)
				{
					canvasPixelPtr = canvasPixelData.get() + (tx + ty * canvasWidth) * bytesPerPixel;
					setPixelColor(canvasPixelPtr, *(canvasBrushPixelPtr + 2), *(canvasBrushPixelPtr + 1),
					              *(canvasBrushPixelPtr), *(canvasBrushPixelPtr + 3));
				}
			}
		}
	}
	if (!UpdateFullRegion)
	{
		//mark this region dirty
		RenderRegions[RegionLookup[pixelCoordX/CellSize][pixelCoordY/CellSize]].isDirty = true;
		//log stuff
		FString logtext = "Writing to region " + FString::FromInt( RegionLookup[pixelCoordX/CellSize][pixelCoordY/CellSize]);
		if(GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, logtext);
	}
}

void UDrawingCanvas::ClearCanvas()
{
	uint8* canvasPixelPtr = canvasPixelData.get();
	for (int i = 0; i < canvasWidth * canvasHeight; ++i)
	{
		setPixelColor(canvasPixelPtr, 255, 255, 255, 255); //white
		canvasPixelPtr += bytesPerPixel;
	}
	//clear the full region
	dynamicCanvas->UpdateTextureRegions((int32)0, 1, FullRegion, (uint32)bufferPitch, (uint32)bytesPerPixel,
													canvasPixelData.get());
}
void UDrawingCanvas::UpdateCanvas()
{
		if (UpdateFullRegion)
		{
			if (FullRegion)
			{
				dynamicCanvas->UpdateTextureRegions((int32)0, 1, FullRegion, (uint32)bufferPitch, (uint32)bytesPerPixel,canvasPixelData.get());
			}
		}
		else
		{
			//TODO::This can be made nicer HM?
			
			//collect all the dirty regions and make a FUpdateTextureRegion2D* we can pass to ->UpdateTextureRegions()
			DirtyRegions = 0;
			for (int i = 0; i < RenderRegions.Num(); i++)
			{
				if(RenderRegions[i].isDirty)
				{
					DirtyUpdateRegions[DirtyRegions] = *RenderRegions[i].region;
					DirtyRegions++;
				}
			}
			//debug stuff
			FString RegionDebug = "Dirty Regions: "+FString::FromInt(DirtyRegions);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *RegionDebug);
			if(GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, RegionDebug);

			//if we have dirty regions render them!
			if(DirtyRegions>0)
				dynamicCanvas->UpdateTextureRegions((int32)0, DirtyRegions, DirtyUpdateRegions, (uint32)bufferPitch, (uint32)bytesPerPixel, canvasPixelData.get());

			//they ain't dirty no more
			for (int i = 0; i < RenderRegions.Num(); ++i)
			{
				RenderRegions[i].isDirty = false;
			}
		}
}
void UDrawingCanvas::setPixelColor(uint8*& pointer, uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	*pointer = blue; //b
	*(pointer + 1) = green; //g
	*(pointer + 2) = red; //r
	*(pointer + 3) = alpha; //a
}
bool UDrawingCanvas::IsPowerOfTwo(int x)
{
	return (x != 0) && ((x & (x - 1)) == 0);
}