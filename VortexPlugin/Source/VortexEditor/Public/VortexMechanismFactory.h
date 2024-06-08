#pragma once
//Copyright(c) 2019 CM Labs Simulations Inc. All rights reserved.
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//the sample code software and associated documentation files (the "Software"), to deal with
//the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
//of the Software, and to permit persons to whom the Software is furnished to
//do so, subject to the following conditions :
//
//Redistributions of source code must retain the above copyright notice,
//this list of conditions and the following disclaimers.
//Redistributions in binary form must reproduce the above copyright notice,
//this list of conditions and the following disclaimers in the documentation
//and/or other materials provided with the distribution.
//Neither the names of CM Labs or Vortex Studio
//nor the names of its contributors may be used to endorse or promote products
//derived from this Software without specific prior written permission.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
//SOFTWARE.
#include "CoreMinimal.h"

#include "Factories/Factory.h"

#include "VortexIntegration/Structs.h"

#include "VortexMechanism.h"
#include "VortexMechanismFactory.generated.h"

//
// uasset factory for vortex mechanisms
//
UCLASS()
class VORTEXEDITOR_API UVortexMechanismFactory : public UFactory
{
    GENERATED_BODY()
public:
    UVortexMechanismFactory(const FObjectInitializer& ObjectInitializer);

    virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    /** Returns true if this factory can deal with the file sent in. */
    virtual bool FactoryCanImport(const FString& Filename) override;

    virtual uint32 GetMenuCategories() const override;

    /** Returns the name of the factory for menus */
    virtual FText GetDisplayName() const override;

private:
    FMechanismGraphicNodeMapping& CreateMappingWithUniqueName(UVortexMechanism* vortexMechanismAsset, VortexGraphicNodeData& nodeData);
};