#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ContractOfferWidget.generated.h"

class UBorder;
class UContractOfferSubsystem;
class UTextBlock;
class UTraderSubsystem;
class UVerticalBox;
struct FSTPContractEffectDefinition;
struct FSTPContractOfferDefinition;
struct FSTPContractTierDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSTPContractTierClickedSignature, FName, OfferId, FName, TierId);

UCLASS()
class SURVIVETHEPLANET_API UContractTierButton : public UButton
{
	GENERATED_BODY()

public:
	void InitializeTier(FName InOfferId, FName InTierId);

	UPROPERTY(BlueprintAssignable, Category = "Contracts")
	FSTPContractTierClickedSignature OnTierClicked;

private:
	FName OfferId;
	FName TierId;

	UFUNCTION()
	void HandleClicked();
};

/** Modal, JSON-driven contract offer popup. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UContractOfferWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Populates and displays the first available offer for TraderId. */
	UFUNCTION(BlueprintCallable, Category = "Contracts")
	bool OpenForTrader(FName TraderId);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UContractOfferSubsystem> ContractSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UTraderSubsystem> TraderSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TraderNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OfferTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> OfferContent;

	FName CurrentOfferId;

	void BuildLayout();
	void PopulateOffer(const FSTPContractOfferDefinition& Offer);
	UBorder* BuildTierCard(const FSTPContractOfferDefinition& Offer, const FSTPContractTierDefinition& Tier);
	void AddEffectSection(UVerticalBox* Parent, const FText& Heading,
		const TArray<FSTPContractEffectDefinition>& Effects, const FLinearColor& HeadingColor);
	static FText FormatEffect(const FSTPContractEffectDefinition& Effect);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleDoneClicked();

	UFUNCTION()
	void HandleTierClicked(FName OfferId, FName TierId);
};
