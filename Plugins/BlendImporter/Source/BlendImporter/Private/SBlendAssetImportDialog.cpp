// // Copyright 2022 nuclearfriend


#include "SBlendAssetImportDialog.h"

#include "BlendAssetFactory.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#if ENGINE_MAJOR_VERSION >= 5
	#include "SWarningOrErrorBox.h"
#endif

#define LOCTEXT_NAMESPACE "BlendImporterModule"

static const ISlateStyle& GetSlateStyle()
{
	#if ENGINE_MAJOR_VERSION < 5
		return FEditorStyle::Get();
	#else
		return FAppStyle::Get();
	#endif
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlendAssetImportDialog::Construct(const FArguments& InArgs)
{
	Collections = InArgs._Collections;

	TSharedPtr<SWidget> MaterialWarning;
	TSharedPtr<SVerticalBox> FilterCollections;

	constexpr double CollectionRowHeight = 23.0;
	constexpr int MaxVisibleCollectionRow = 26;
	constexpr double FixedHeight = 220;
	
	double ClientHeight = FixedHeight + CollectionRowHeight * FMath::Clamp(Collections.Num(), 1, MaxVisibleCollectionRow);
	double MaxHeight = FixedHeight + MaxVisibleCollectionRow * CollectionRowHeight + 40;

	if (InArgs._ShowMaterialWarning)
	{
		ClientHeight += 80;
		MaxHeight += 80;
	}

	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("SBlendAssetImportDialog_Title", "Blend Import Options"))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.ClientSize(FVector2D(400, ClientHeight))
		.MinHeight(ClientHeight)
		.MaxHeight(MaxHeight)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 5, 0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.MaxWidth(28.0f)
				[
					SNew(SBox)
					.Padding(6)
					[
						SNew(SImage)
						#if ENGINE_MAJOR_VERSION < 5
							.Image(FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.ImportIcon").GetIcon())
						#else
							.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import").GetIcon())
						#endif
					]
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredHeight(28.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(InArgs._Filename)
						.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
					]
				]
			]
			
			+SVerticalBox::Slot()
			.AutoHeight()
			[				
				SNew(SBorder)
				.BorderImage(GetSlateStyle().GetBrush("DetailsView.CategoryTop"))
				.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(SBox)
					.MinDesiredHeight(16.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("General"))
						.Font(GetSlateStyle().GetFontStyle("DetailsView.CategoryFontStyle"))
						.TextStyle(GetSlateStyle(), "DetailsView.CategoryTextStyle")
					]
				]
			]
			
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(SHorizontalBox)
				.ToolTipText(FText::FromString("Import meshes with their Object origin (rather than World)."))
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Use object origin"))
					.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda(
						[this]() -> ECheckBoxState {
							return bUseObjectPivot ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						}
					)
					.OnCheckStateChanged_Lambda(
						[this](ECheckBoxState NewState) {
							bUseObjectPivot = (NewState == ECheckBoxState::Checked);
						}
					)
				]
			]
			
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(SHorizontalBox)
				.ToolTipText(FText::FromString("If your Blender scene has meshes with child meshes, this will merge them together and import them as a single mesh"))
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Merge child meshes with parents"))
					.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda(
						[this]() -> ECheckBoxState {
							return bMergeChildMeshes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						}
					)
					.OnCheckStateChanged_Lambda(
						[this](ECheckBoxState NewState) {
							bMergeChildMeshes = (NewState == ECheckBoxState::Checked);
						}
					)
				]				
			]
			
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(GetSlateStyle().GetBrush("DetailsView.CategoryTop"))
				.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(SBox)
					.MinDesiredHeight(16.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Collections"))
						.Font(GetSlateStyle().GetFontStyle("DetailsView.CategoryFontStyle"))
						.TextStyle(GetSlateStyle(), "DetailsView.CategoryTextStyle")
					]
				]
			]

			+SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			.FillHeight(1.0)
			.MaxHeight(MaxVisibleCollectionRow * CollectionRowHeight)
			.Padding(5)
			[
				SNew(SScrollBox)
				+SScrollBox::Slot()
				[
					SAssignNew(FilterCollections, SVerticalBox)
				]
			]

			+SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.AutoHeight()
			.Padding(5)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					#if ENGINE_MAJOR_VERSION >= 5
						SAssignNew(MaterialWarning, SWarningOrErrorBox)
						.MessageStyle(EMessageStyle::Warning)
						.Message(FText::FromString("This file has issues which may make the materials import incorrectly. Check the \"Blend Importer\" section of the Message Log for more details."))
					#else
						SAssignNew(MaterialWarning, STextBlock)
						.Text(FText::FromString("This file has issues which may make the materials import incorrectly. Check the \"Blend Importer\" section of the Message Log for more details."))
						.ColorAndOpacity(FLinearColor::Yellow)
						.AutoWrapText(true)
					#endif
				]

				+SVerticalBox::Slot()
				.HAlign(HAlign_Right)
				.FillHeight(1.0f)
				.Padding(5)
				[
					SNew(SUniformGridPanel)
					// .SlotPadding(GetSlateStyle().GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(GetSlateStyle().GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(GetSlateStyle().GetFloat("StandardDialog.MinDesiredSlotHeight"))
					+SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(GetSlateStyle().GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("OK", "OK"))
						.OnClicked(this, &SBlendAssetImportDialog::OnButtonClick, EAppReturnType::Ok)
					]
					+SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(GetSlateStyle().GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("Cancel", "Cancel"))
						.OnClicked(this, &SBlendAssetImportDialog::OnButtonClick, EAppReturnType::Cancel)
					]
				]
			]			
		]);

	MaterialWarning->SetVisibility(InArgs._ShowMaterialWarning ? EVisibility::Visible : EVisibility::Collapsed);
	
	bUseObjectPivot = InArgs._PreviousOptions->bUseObjectPivot;
	bMergeChildMeshes = InArgs._PreviousOptions->bMergeChildMeshes;

	if (InArgs._Collections.Num() > 0)
	{
		bool bSimilarCollections = false;
		for (int32 i = 0; i < InArgs._Collections.Num(); i++)
		{
			if (InArgs._PreviousOptions->EnabledCollections.Find(InArgs._Collections[i]) != INDEX_NONE)
			{
				bSimilarCollections = true;
				break;
			}
		}
		
		for (int32 i = 0; i < InArgs._Collections.Num(); i++)
		{
			TSharedPtr<SCheckBox> CollectionCheckbox;

			FilterCollections->AddSlot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Margin(FMargin(10.f, 5.f, 0.f, 5.f))
					.Text(FText::FromString(InArgs._Collections[i]))
					.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				[
					SAssignNew(CollectionCheckbox, SCheckBox)
					.OnCheckStateChanged(this, &SBlendAssetImportDialog::OnCheckboxCollectionChanged, i)
				]
			];

			bool IsChecked = false;
			if (bSimilarCollections)
			{
				if (InArgs._PreviousOptions->EnabledCollections.Find(InArgs._Collections[i]) != INDEX_NONE)
				{
					IsChecked = true;
				}
			}
			else
			{
				IsChecked = true;
			}
			if (IsChecked)
			{
				EnabledCollections.Add(InArgs._Collections[i]);
				CollectionCheckbox->SetIsChecked(ECheckBoxState::Checked);
			}
		}
	}
	else
	{
		FilterCollections->AddSlot()
		[
			SNew(STextBlock)
			.Margin(FMargin(10.f, 5.f, 0.f, 5.f))
			.Text(FText::FromString("No collections defined, whole file will be imported"))
			.Font(GetSlateStyle().GetFontStyle("PropertyWindow.NormalFont"))
			.ColorAndOpacity(FSlateColor(FSlateColor::UseSubduedForeground()))
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

EAppReturnType::Type SBlendAssetImportDialog::ShowModal()
{
	GEditor->EditorAddModalWindow(SharedThis(this));
	return UserResponse;
}

bool SBlendAssetImportDialog::IsUseObjectPivot() const
{
	return bUseObjectPivot;
}

TArray<FString> SBlendAssetImportDialog::GetEnabledCollections() const
{
	return EnabledCollections;
}

bool SBlendAssetImportDialog::IsMergeChildMeshes() const
{
	return bMergeChildMeshes;
}

FReply SBlendAssetImportDialog::OnButtonClick(EAppReturnType::Type ButtonID)
{
	UserResponse = ButtonID;
	RequestDestroyWindow();

	return FReply::Handled();
}

void SBlendAssetImportDialog::OnCheckboxCollectionChanged(ECheckBoxState InCheckState, int32 Idx)
{
	if (InCheckState == ECheckBoxState::Checked)
	{
		EnabledCollections.Add(Collections[Idx]);
	}
	else
	{
		EnabledCollections.Remove(Collections[Idx]);
	}
}

ECheckBoxState SBlendAssetImportDialog::IsCheckboxOptionChecked(int OptionID) const
{
	if (OptionID == 1)
	{
		return bUseObjectPivot ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

void SBlendAssetImportDialog::OnCheckboxOptionChanged(ECheckBoxState InCheckState, int32 OptionID)
{
	if (OptionID == 1)
	{
		bUseObjectPivot = InCheckState == ECheckBoxState::Checked;
	}
}
