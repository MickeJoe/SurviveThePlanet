#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "TraderPanelWidget.generated.h"

class AResourceManager;
class UBorder;
class UButton;
class UTextBlock;
class UTraderSubsystem;
class UContractOfferSubsystem;
class UContractOfferWidget;
class UVerticalBox;
struct FSTPTraderDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSTPTraderSelectedSignature, FName, TraderId);

UCLASS()
class SURVIVETHEPLANET_API UTraderOfferButton : public UButton
{
	GENERATED_BODY()

public:
	void InitializeTrader(FName InTraderId);

	UPROPERTY(BlueprintAssignable, Category = "Trading")
	FSTPTraderSelectedSignature OnTraderClicked;

private:
	FName TraderId;

	UFUNCTION()
	void HandleClicked();
};

/** Runtime trader overview. Can be used directly or as the native parent of WBP_TraderPanel. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UTraderPanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	/** Fired by VIEW OFFERS. Bind this in WBP to open the matching contract popup. */
	UPROPERTY(BlueprintAssignable, Category = "Trading")
	FSTPTraderSelectedSignature OnViewOffersRequested;

	/** Optional WBP-authored list. A complete fallback layout is built when it is absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Trading")
	TObjectPtr<UVerticalBox> TraderList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Trading")
	TObjectPtr<UTextBlock> TradersHeading;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTraderSubsystem> TraderSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UContractOfferSubsystem> ContractSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UContractOfferWidget> ContractOfferPopup;

	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ContentBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CollapseGlyph;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ContractsPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContractsList;

	bool bCollapsed = false;
	float TimerRefreshAccumulator = 0.0f;
	float ActiveContractSimulationAccumulator = 0.0f;
	TMap<FName, float> OfferSecondsRemaining;
	TSet<FName> PreviouslyEligibleTraders;
	TSet<FName> ExpiredTraders;
	FName TrackedContractOfferId;
	FName TrackedContractTierId;
	float NextDeliverySecondsRemaining = 0.0f;

	void BuildFallbackLayout();
	void ResolveResourceManager();
	void RefreshTraders();
	void RefreshActiveContracts(float DeltaTime);
	bool IsTraderEligible(const FSTPTraderDefinition& Trader) const;
	void UpdateOfferStates(float DeltaTime);
	static FText FormatOfferTime(float RemainingSeconds);

	UFUNCTION()
	void HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount);

	UFUNCTION()
	void HandleCollapseClicked();

	UFUNCTION()
	void HandleViewOffersRequested(FName TraderId);
};
