/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */


#include <memory>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <vcl/window.hxx>

#include <editeng/lspcitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/contouritem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/adjustitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/shdditem.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/kernitem.hxx>
#include <editeng/wrlmitem.hxx>
#include <editeng/autokernitem.hxx>
#include <editeng/langitem.hxx>
#include <editeng/emphasismarkitem.hxx>
#include <editeng/numitem.hxx>
#include <editeng/tstpitem.hxx>
#include <editeng/charscaleitem.hxx>
#include <editeng/charreliefitem.hxx>
#include <editeng/frmdiritem.hxx>

#include "impedit.hxx"
#include <editeng/editeng.hxx>
#include <editeng/editview.hxx>
#include <editdoc.hxx>

#include <rtl/strbuf.hxx>
#include <osl/diagnose.h>

#if defined( DBG_UTIL ) || ( OSL_DEBUG_LEVEL > 1 )

static OString DbgOutItem(const SfxItemPool& rPool, const SfxPoolItem& rItem)
{
    OStringBuffer aDebStr;
    switch ( rItem.Which() )
    {
        case EE_PARA_WRITINGDIR:
            aDebStr.append("WritingDir=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxFrameDirectionItem&>(rItem).GetValue()));
        break;
        case EE_PARA_OUTLLRSPACE:
        case EE_PARA_LRSPACE:
            aDebStr.append("FI=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxLRSpaceItem&>(rItem).GetTextFirstLineOffset()));
            aDebStr.append(", LI=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxLRSpaceItem&>(rItem).GetTextLeft()));
            aDebStr.append(", RI=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxLRSpaceItem&>(rItem).GetRight()));
        break;
        case EE_PARA_NUMBULLET:
            aDebStr.append("NumItem ");
            for ( sal_uInt16 nLevel = 0; nLevel < 3; nLevel++ )
            {
                aDebStr.append("Level");
                aDebStr.append(static_cast<sal_Int32>(nLevel));
                aDebStr.append('=');
                const SvxNumberFormat* pFmt = static_cast<const SvxNumBulletItem&>(rItem).GetNumRule().Get( nLevel );
                if ( pFmt )
                {
                    aDebStr.append('(');
                    aDebStr.append(pFmt->GetFirstLineOffset());
                    aDebStr.append(',');
                    aDebStr.append(pFmt->GetAbsLSpace());
                    aDebStr.append(',');
                    if ( pFmt->GetNumberingType() == SVX_NUM_BITMAP )
                        aDebStr.append("Bitmap");
                    else if( pFmt->GetNumberingType() != SVX_NUM_CHAR_SPECIAL )
                        aDebStr.append("Number");
                    else
                    {
                        aDebStr.append("Char=[");
                        aDebStr.append(static_cast<sal_Int32>(pFmt->GetBulletChar()));
                        aDebStr.append(']');
                    }
                    aDebStr.append(") ");
                }
            }
        break;
        case EE_PARA_BULLETSTATE:
            aDebStr.append("ShowBullet=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SfxBoolItem&>(rItem).GetValue()));
        break;
        case EE_PARA_HYPHENATE:
            aDebStr.append("Hyphenate=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SfxBoolItem&>(rItem).GetValue()));
        break;
        case EE_PARA_OUTLLEVEL:
            aDebStr.append("Level=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SfxInt16Item&>(rItem).GetValue()));
        break;
        case EE_PARA_ULSPACE:
            aDebStr.append("SB=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxULSpaceItem&>(rItem).GetUpper()));
            aDebStr.append(", SA=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxULSpaceItem&>(rItem).GetLower()));
        break;
        case EE_PARA_SBL:
            aDebStr.append("SBL=");
            if ( static_cast<const SvxLineSpacingItem&>(rItem).GetLineSpaceRule() == SvxLineSpaceRule::Min )
            {
                aDebStr.append("Min: ");
                aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxLineSpacingItem&>(rItem).GetInterLineSpace()));
            }
            else if ( static_cast<const SvxLineSpacingItem&>(rItem).GetInterLineSpaceRule() == SvxInterLineSpaceRule::Prop )
            {
                aDebStr.append("Prop: ");
                aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxLineSpacingItem&>(rItem).GetPropLineSpace()));
            }
            else
                aDebStr.append("Unsupported Type!");
        break;
        case EE_PARA_JUST:
            aDebStr.append("SvxAdust=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxAdjustItem&>(rItem).GetAdjust()));
        break;
        case EE_PARA_TABS:
        {
            aDebStr.append("Tabs: ");
            const SvxTabStopItem& rTabs = static_cast<const SvxTabStopItem&>(rItem);
            aDebStr.append(static_cast<sal_Int32>(rTabs.Count()));
            if ( rTabs.Count() )
            {
                aDebStr.append("( ");
                for (sal_uInt16 i = 0; i < rTabs.Count(); ++i)
                {
                    const SvxTabStop& rTab = rTabs[i];
                    aDebStr.append(rTab.GetTabPos());
                    aDebStr.append(' ');
                }
                aDebStr.append(')');
            }
        }
        break;
        case EE_CHAR_LANGUAGE:
        case EE_CHAR_LANGUAGE_CJK:
        case EE_CHAR_LANGUAGE_CTL:
            aDebStr.append("Language=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<sal_uInt16>(static_cast<const SvxLanguageItem&>(rItem).GetLanguage())));
        break;
        case EE_CHAR_COLOR:
        {
            aDebStr.append("Color= ");
            Color aColor( static_cast<const SvxColorItem&>(rItem).GetValue() );
            aDebStr.append(static_cast<sal_Int32>(aColor.GetRed()));
            aDebStr.append(", ");
            aDebStr.append(static_cast<sal_Int32>(aColor.GetGreen()));
            aDebStr.append(", ");
            aDebStr.append(static_cast<sal_Int32>(aColor.GetBlue()));
        }
        break;
        case EE_CHAR_BKGCOLOR:
        {
            aDebStr.append("FillColor= ");
            Color aColor( static_cast<const SvxColorItem&>(rItem).GetValue() );
            aDebStr.append(static_cast<sal_Int32>(aColor.GetRed()));
            aDebStr.append(", ");
            aDebStr.append(static_cast<sal_Int32>(aColor.GetGreen()));
            aDebStr.append(", ");
            aDebStr.append(static_cast<sal_Int32>(aColor.GetBlue()));
        }
        break;
        case EE_CHAR_FONTINFO:
        case EE_CHAR_FONTINFO_CJK:
        case EE_CHAR_FONTINFO_CTL:
        {
            aDebStr.append("Font=");
            aDebStr.append(OUStringToOString(static_cast<const SvxFontItem&>(rItem).GetFamilyName(), RTL_TEXTENCODING_ASCII_US));
            aDebStr.append(" (CharSet: ");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxFontItem&>(rItem).GetCharSet()));
            aDebStr.append(')');
        }
        break;
        case EE_CHAR_FONTHEIGHT:
        case EE_CHAR_FONTHEIGHT_CJK:
        case EE_CHAR_FONTHEIGHT_CTL:
        {
            aDebStr.append("Groesse=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxFontHeightItem&>(rItem).GetHeight()));
            Size aSz( 0, static_cast<const SvxFontHeightItem&>(rItem).GetHeight() );
            MapUnit eUnit = rPool.GetMetric( rItem.Which() );
            MapMode aItemMapMode(eUnit);
            MapMode aPntMap( MapUnit::MapPoint );
            aSz = OutputDevice::LogicToLogic( aSz, aItemMapMode, aPntMap );
            aDebStr.append(" Points=");
            aDebStr.append(static_cast<sal_Int32>(aSz.Height()));
        }
        break;
        case EE_CHAR_FONTWIDTH:
        {
            aDebStr.append("Breite=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxCharScaleWidthItem&>(rItem).GetValue()));
            aDebStr.append('%');
        }
        break;
        case EE_CHAR_WEIGHT:
        case EE_CHAR_WEIGHT_CJK:
        case EE_CHAR_WEIGHT_CTL:
            aDebStr.append("FontWeight=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxWeightItem&>(rItem).GetWeight()));
        break;
        case EE_CHAR_UNDERLINE:
            aDebStr.append("FontUnderline=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxUnderlineItem&>(rItem).GetLineStyle()));
        break;
        case EE_CHAR_OVERLINE:
            aDebStr.append("FontOverline=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxOverlineItem&>(rItem).GetLineStyle()));
        break;
        case EE_CHAR_EMPHASISMARK:
            aDebStr.append("FontUnderline=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxEmphasisMarkItem&>(rItem).GetEmphasisMark()));
        break;
        case EE_CHAR_RELIEF:
            aDebStr.append("FontRelief=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxCharReliefItem&>(rItem).GetValue()));
        break;
        case EE_CHAR_STRIKEOUT:
            aDebStr.append("FontStrikeout=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxCrossedOutItem&>(rItem).GetStrikeout()));
        break;
        case EE_CHAR_ITALIC:
        case EE_CHAR_ITALIC_CJK:
        case EE_CHAR_ITALIC_CTL:
            aDebStr.append("FontPosture=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxPostureItem&>(rItem).GetPosture()));
        break;
        case EE_CHAR_OUTLINE:
            aDebStr.append("FontOutline=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxContourItem&>(rItem).GetValue()));
        break;
        case EE_CHAR_SHADOW:
            aDebStr.append("FontShadowed=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxShadowedItem&>(rItem).GetValue()));
        break;
        case EE_CHAR_ESCAPEMENT:
            aDebStr.append("Escape=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxEscapementItem&>(rItem).GetEsc()));
            aDebStr.append(", ");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxEscapementItem&>(rItem).GetProportionalHeight()));
        break;
        case EE_CHAR_PAIRKERNING:
            aDebStr.append("PairKerning=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxAutoKernItem&>(rItem).GetValue()));
        break;
        case EE_CHAR_KERNING:
        {
            aDebStr.append("Kerning=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxKerningItem&>(rItem).GetValue()));
            Size aSz( 0, static_cast<short>(static_cast<const SvxKerningItem&>(rItem).GetValue()) );
            MapUnit eUnit = rPool.GetMetric( rItem.Which() );
            MapMode aItemMapMode(eUnit);
            MapMode aPntMap( MapUnit::MapPoint );
            aSz = OutputDevice::LogicToLogic( aSz, aItemMapMode, aPntMap );
            aDebStr.append(" Points=");
            aDebStr.append(static_cast<sal_Int32>(aSz.Height()));
        }
        break;
        case EE_CHAR_WLM:
            aDebStr.append("WordLineMode=");
            aDebStr.append(static_cast<sal_Int32>(static_cast<const SvxWordLineModeItem&>(rItem).GetValue()));
        break;
        case EE_CHAR_XMLATTRIBS:
            aDebStr.append("XMLAttribs=...");
        break;
    }
    return aDebStr.makeStringAndClear();
}

static void DbgOutItemSet(FILE* fp, const SfxItemSet& rSet, bool bSearchInParent, bool bShowALL)
{
    for ( sal_uInt16 nWhich = EE_PARA_START; nWhich <= EE_CHAR_END; nWhich++ )
    {
        fprintf( fp, "\nWhich: %i\t", nWhich );
        if ( rSet.GetItemState( nWhich, bSearchInParent ) == SfxItemState::DEFAULT )
            fprintf( fp, "ITEM_OFF   " );
        else if ( rSet.GetItemState( nWhich, bSearchInParent ) == SfxItemState::DONTCARE )
            fprintf( fp, "ITEM_DC    " );
        else if ( rSet.GetItemState( nWhich, bSearchInParent ) == SfxItemState::SET )
            fprintf( fp, "ITEM_ON   *" );

        if ( !bShowALL && ( rSet.GetItemState( nWhich, bSearchInParent ) != SfxItemState::SET ) )
            continue;

        const SfxPoolItem& rItem = rSet.Get( nWhich, bSearchInParent );
        OString aDebStr = DbgOutItem( *rSet.GetPool(), rItem );
        fprintf( fp, "%s", aDebStr.getStr() );
    }
}

void EditEngine::DumpData(const EditEngine* pEE, bool bInfoBox)
{
    if (!pEE)
        return;

    FILE* fp = fopen( "editenginedump.log", "w" );
    if ( fp == nullptr )
    {
        OSL_FAIL( "Log file could not be created!" );
        return;
    }

    const SfxItemPool& rPool = *pEE->GetEmptyItemSet().GetPool();

    fprintf( fp, "================================================================================" );
    fprintf( fp, "\n==================   Document   ================================================" );
    fprintf( fp, "\n================================================================================" );
    for ( sal_Int32 nPortion = 0; nPortion < pEE->pImpEditEngine->GetParaPortions().Count(); nPortion++)
    {
        ParaPortion& rPPortion = pEE->pImpEditEngine->GetParaPortions()[nPortion];
        fprintf( fp, "\nParagraph %" SAL_PRIdINT32 ": Length = %" SAL_PRIdINT32 ", Invalid = %i\nText = '%s'",
                 nPortion, rPPortion.GetNode()->Len(), rPPortion.IsInvalid(),
                 OUStringToOString(rPPortion.GetNode()->GetString(), RTL_TEXTENCODING_UTF8).getStr() );
        fprintf( fp, "\nVorlage:" );
        SfxStyleSheet* pStyle = rPPortion.GetNode()->GetStyleSheet();
        if ( pStyle )
            fprintf( fp, " %s", OUStringToOString( pStyle->GetName(), RTL_TEXTENCODING_UTF8).getStr() );
        fprintf( fp, "\nParagraph attribute:" );
        DbgOutItemSet( fp, rPPortion.GetNode()->GetContentAttribs().GetItems(), false, false );

        fprintf( fp, "\nCharacter attribute:" );
        bool bZeroAttr = false;
        for ( sal_Int32 z = 0; z < rPPortion.GetNode()->GetCharAttribs().Count(); ++z )
        {
            const std::unique_ptr<EditCharAttrib>& rAttr = rPPortion.GetNode()->GetCharAttribs().GetAttribs()[z];
            OStringBuffer aCharAttribs;
            aCharAttribs.append("\nA");
            aCharAttribs.append(nPortion);
            aCharAttribs.append(":  ");
            aCharAttribs.append(static_cast<sal_Int32>(rAttr->GetItem()->Which()));
            aCharAttribs.append('\t');
            aCharAttribs.append(rAttr->GetStart());
            aCharAttribs.append('\t');
            aCharAttribs.append(rAttr->GetEnd());
            if ( rAttr->IsEmpty() )
                bZeroAttr = true;
            fprintf(fp, "%s => ", aCharAttribs.getStr());

            OString aDebStr = DbgOutItem( rPool, *rAttr->GetItem() );
            fprintf( fp, "%s", aDebStr.getStr() );
        }
        if ( bZeroAttr )
            fprintf( fp, "\nNULL-Attribute!" );

        const sal_Int32 nTextPortions = rPPortion.GetTextPortions().Count();
        OStringBuffer aPortionStr("\nText portions: #");
        aPortionStr.append(nTextPortions);
        aPortionStr.append(" \nA");
        aPortionStr.append(nPortion);
        aPortionStr.append(": Paragraph Length = ");
        aPortionStr.append(rPPortion.GetNode()->Len());
        aPortionStr.append("\nA");
        aPortionStr.append(nPortion);
        aPortionStr.append(": ");
        sal_Int32 n = 0;
        for ( sal_Int32 z = 0; z < nTextPortions; ++z )
        {
            TextPortion& rPortion = rPPortion.GetTextPortions()[z];
            aPortionStr.append(' ');
            aPortionStr.append(rPortion.GetLen());
            aPortionStr.append('(');
            aPortionStr.append(static_cast<sal_Int32>(rPortion.GetSize().Width()));
            aPortionStr.append(')');
            aPortionStr.append('[');
            aPortionStr.append(static_cast<sal_Int32>(rPortion.GetKind()));
            aPortionStr.append(']');
            aPortionStr.append(';');
            n += rPortion.GetLen();
        }
        aPortionStr.append("\nA");
        aPortionStr.append(nPortion);
        aPortionStr.append(": Total length: ");
        aPortionStr.append(n);
        if ( rPPortion.GetNode()->Len() != n )
            aPortionStr.append(" => Error !!!");
        fprintf(fp, "%s", aPortionStr.getStr());

        fprintf( fp, "\n\nLines:" );
        // First the content ...
        for ( sal_Int32 nLine = 0; nLine < rPPortion.GetLines().Count(); nLine++ )
        {
            EditLine& rLine = rPPortion.GetLines()[nLine];

            OString aLine(OUStringToOString(rPPortion.GetNode()->Copy(rLine.GetStart(), rLine.GetEnd() - rLine.GetStart()), RTL_TEXTENCODING_ASCII_US));
            fprintf( fp, "\nLine %" SAL_PRIdINT32 "\t>%s<", nLine, aLine.getStr() );
        }
        // then the internal data ...
        for ( sal_Int32 nLine = 0; nLine < rPPortion.GetLines().Count(); nLine++ )
        {
            EditLine& rLine = rPPortion.GetLines()[nLine];
            fprintf( fp, "\nLine %" SAL_PRIdINT32 ":\tStart: %" SAL_PRIdINT32 ",\tEnd: %" SAL_PRIdINT32, nLine, rLine.GetStart(), rLine.GetEnd() );
            fprintf( fp, "\t\tPortions: %" SAL_PRIdINT32 " - %" SAL_PRIdINT32 ".\tHight: %i, Ascent=%i", rLine.GetStartPortion(), rLine.GetEndPortion(), rLine.GetHeight(), rLine.GetMaxAscent() );
        }

        fprintf( fp, "\n-----------------------------------------------------------------------------" );
    }

    if ( pEE->pImpEditEngine->GetStyleSheetPool() )
    {
        SfxStyleSheetIterator aIter( pEE->pImpEditEngine->GetStyleSheetPool(), SfxStyleFamily::All );
        sal_uInt16 nStyles = aIter.Count();
        fprintf( fp, "\n\n================================================================================" );
        fprintf( fp, "\n==================   Stylesheets   =============================================" );
        fprintf( fp, "\n================================================================================" );
        fprintf( fp, "\n#Template:   %" SAL_PRIuUINT32 "\n", sal_uInt32(nStyles) );
        SfxStyleSheetBase* pStyle = aIter.First();
        while ( pStyle )
        {
            fprintf( fp, "\nTemplate:   %s", OUStringToOString( pStyle->GetName(), RTL_TEXTENCODING_ASCII_US ).getStr() );
            fprintf( fp, "\nParent:    %s", OUStringToOString( pStyle->GetParent(), RTL_TEXTENCODING_ASCII_US ).getStr() );
            fprintf( fp, "\nFollow:    %s", OUStringToOString( pStyle->GetFollow(), RTL_TEXTENCODING_ASCII_US ).getStr() );
            DbgOutItemSet( fp, pStyle->GetItemSet(), false, false );
            fprintf( fp, "\n----------------------------------" );

            pStyle = aIter.Next();
        }
    }

    fprintf( fp, "\n\n================================================================================" );
    fprintf( fp, "\n==================   Defaults   ================================================" );
    fprintf( fp, "\n================================================================================" );
    DbgOutItemSet( fp, pEE->pImpEditEngine->GetEmptyItemSet(), true, true );

    fprintf( fp, "\n\n================================================================================" );
    fprintf( fp, "\n==================   EditEngine & Views   ======================================" );
    fprintf( fp, "\n================================================================================" );
    fprintf( fp, "\nControl: %x", unsigned( pEE->GetControlWord() ) );
    fprintf( fp, "\nRefMapMode: %i", int( pEE->pImpEditEngine->pRefDev->GetMapMode().GetMapUnit() ) );
    fprintf( fp, "\nPaperSize: %" SAL_PRIdINT64 " x %" SAL_PRIdINT64, sal_Int64(pEE->GetPaperSize().Width()), sal_Int64(pEE->GetPaperSize().Height()) );
    fprintf( fp, "\nMaxAutoPaperSize: %" SAL_PRIdINT64 " x %" SAL_PRIdINT64, sal_Int64(pEE->GetMaxAutoPaperSize().Width()), sal_Int64(pEE->GetMaxAutoPaperSize().Height()) );
    fprintf( fp, "\nMinAutoPaperSize: %" SAL_PRIdINT64 " x %" SAL_PRIdINT64 , sal_Int64(pEE->GetMinAutoPaperSize().Width()), sal_Int64(pEE->GetMinAutoPaperSize().Height()) );
    fprintf( fp, "\nCalculateLayout: %i", pEE->IsUpdateLayout() );
    fprintf( fp, "\nNumber of Views: %" SAL_PRI_SIZET "i", pEE->GetViewCount() );
    for ( size_t nView = 0; nView < pEE->GetViewCount(); nView++ )
    {
        EditView* pV = pEE->GetView( nView );
        DBG_ASSERT( pV, "View not found!" );
        fprintf( fp, "\nView %zu: Focus=%i", nView, pV->GetWindow()->HasFocus() );
        tools::Rectangle aR( pV->GetOutputArea() );
        fprintf( fp, "\n  OutputArea: nX=%" SAL_PRIdINT64 ", nY=%" SAL_PRIdINT64 ", dX=%" SAL_PRIdINT64 ", dY=%" SAL_PRIdINT64 ", MapMode = %i",
            sal_Int64(aR.Left()), sal_Int64(aR.Top()), sal_Int64(aR.GetSize().Width()), sal_Int64(aR.GetSize().Height()) , int( pV->GetWindow()->GetMapMode().GetMapUnit() ) );
        aR = pV->GetVisArea();
        fprintf( fp, "\n  VisArea: nX=%" SAL_PRIdINT64 ", nY=%" SAL_PRIdINT64 ", dX=%" SAL_PRIdINT64 ", dY=%" SAL_PRIdINT64,
            sal_Int64(aR.Left()), sal_Int64(aR.Top()), sal_Int64(aR.GetSize().Width()), sal_Int64(aR.GetSize().Height()) );
        ESelection aSel = pV->GetSelection();
        fprintf( fp, "\n  Selection: Start=%" SAL_PRIdINT32 ",%" SAL_PRIdINT32 ", End=%" SAL_PRIdINT32 ",%" SAL_PRIdINT32, aSel.nStartPara, aSel.nStartPos, aSel.nEndPara, aSel.nEndPos );
    }
    if ( pEE->GetActiveView() )
    {
        fprintf( fp, "\n\n================================================================================" );
        fprintf( fp, "\n==================   Current View   ===========================================" );
        fprintf( fp, "\n================================================================================" );
        DbgOutItemSet( fp, pEE->GetActiveView()->GetAttribs(), true, false );
    }
    fclose( fp );
    if ( bInfoBox )
    {
        std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(nullptr,
                                                      VclMessageType::Info, VclButtonsType::Ok,
                                                      "Dumped editenginedump.log!" ));
        xInfoBox->run();
    }
}
#endif

#if OSL_DEBUG_LEVEL > 0
bool ParaPortion::DbgCheckTextPortions(ParaPortion const& rPara)
{
    // check, if Portion length ok:
    sal_uInt16 nXLen = 0;
    for (sal_Int32 nPortion = 0; nPortion < rPara.aTextPortionList.Count(); nPortion++)
    {
        nXLen = nXLen + rPara.aTextPortionList[nPortion].GetLen();
    }
    return nXLen == rPara.pNode->Len();
}
#endif

#if OSL_DEBUG_LEVEL > 0 && !defined NDEBUG
void CheckOrderedList(const CharAttribList::AttribsType& rAttribs)
{
    sal_Int32 nPrev = 0;
    for (const std::unique_ptr<EditCharAttrib>& rAttr : rAttribs)
    {
        sal_Int32 const nCur = rAttr->GetStart();
        assert(nCur >= nPrev);
        nPrev = nCur;
    }
}
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
