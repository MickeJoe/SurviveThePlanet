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
class UObjectiveTrackerWidget;
struct FSTPTraderDefinition;
struct FSTPContractOfferDefinition;
struct FSTPContractTierDefinition;

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

USTRUCT()
struct FTraderResourceTextBinding
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<UTextBlock> Text;
	UPROPERTY() EResourceType Resource = EResourceType::Iron;
	UPROPERTY() int32 RequiredAmount = 0;
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

	UPROPERTY(Transient) TObjectPtr<UTextBlock> ActiveProgressText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ActiveDeadlineText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ActiveDeliveryStatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ActiveRewardText;
	UPROPERTY(Transient) TObjectPtr<UButton> ActiveDeliverButton;
	UPROPERTY(Transient) TArray<FTraderResourceTextBinding> TraderResourceTexts;
	FName ActiveOfferId;
	FName ActiveTierId;
	void UpdateTraderResourceTexts(EResourceType Resource);
	TWeakObjectPtr<UObjectiveTrackerWidget> ObjectiveTrackerWidget;
	bool bCollapsed = false;
	bool bTraderStructureDirty = false;
	float TimerRefreshAccumulator = 0.0f;
	FVector2D LastPanelPosition = FVector2D(-1.0f);
	FVector2D LastPanelSize = FVector2D::ZeroVector;
	TMap<FName, float> OfferSecondsRemaining;
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTextBlock>> OfferTimerTexts;
	TSet<FName> PreviouslyEligibleTraders;
	TSet<FName> ExpiredTraders;


	void BuildFallbackLayout();
	void ResolveResourceManager();
	void RefreshTraders();
	void UpdateTraderTimerTexts();
	void RefreshActiveContracts();
	void BuildActiveContractWidgets(const FSTPContractOfferDefinition& Offer, const FSTPContractTierDefinition& Tier);
	UFUNCTION() void HandleContractStateChanged();
	UFUNCTION() void HandleDeliverContractClicked();
	bool IsTraderEligible(const FSTPTraderDefinition& Trader) const;
	bool UpdateOfferStates(float DeltaTime);
	static FText FormatOfferTime(float RemainingSeconds);

	UFUNCTION()
	void HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount);

	UFUNCTION()
	void HandleCollapseClicked();

	UFUNCTION()
	void HandleViewOffersRequested(FName TraderId);
};
