/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "eemd.hxx"

#include <editeng/editeng.hxx>
#include <editeng/flditem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/numitem.hxx>
#include <editeng/eeitem.hxx>
#include <svl/intitem.hxx>

EditMDParser::EditMDParser(EditEngine* pEditEngine, const EditPaM& rPaM)
    : mpEditEngine(pEditEngine)
    , maCurSel(rPaM)
    , mbInParagraph(false)
    , mbNeedParaBreak(false)
    , mnListDepth(-1)
    , mbBold(false)
    , mbItalic(false)
    , mbStrikethrough(false)
    , mbCode(false)
    , mnLinkStart(-1)
{
}

EditPaM EditMDParser::Parse(const OString& rMarkdown)
{
    MD_PARSER parser;
    memset(&parser, 0, sizeof(parser));
    parser.abi_version = 0;
    parser.flags = MD_DIALECT_GITHUB;
    parser.enter_block = EnterBlockCb;
    parser.leave_block = LeaveBlockCb;
    parser.enter_span = EnterSpanCb;
    parser.leave_span = LeaveSpanCb;
    parser.text = TextCb;
    parser.debug_log = nullptr;
    parser.syntax = nullptr;

    md_parse(rMarkdown.getStr(), rMarkdown.getLength(), &parser, this);

    return maCurSel.Max();
}

int EditMDParser::EnterBlockCb(MD_BLOCKTYPE nType, void* pDetail, void* pUserData)
{
    static_cast<EditMDParser*>(pUserData)->EnterBlock(nType, pDetail);
    return 0;
}

int EditMDParser::LeaveBlockCb(MD_BLOCKTYPE nType, void* pDetail, void* pUserData)
{
    static_cast<EditMDParser*>(pUserData)->LeaveBlock(nType, pDetail);
    return 0;
}

int EditMDParser::EnterSpanCb(MD_SPANTYPE nType, void* pDetail, void* pUserData)
{
    static_cast<EditMDParser*>(pUserData)->EnterSpan(nType, pDetail);
    return 0;
}

int EditMDParser::LeaveSpanCb(MD_SPANTYPE nType, void* pDetail, void* pUserData)
{
    static_cast<EditMDParser*>(pUserData)->LeaveSpan(nType, pDetail);
    return 0;
}

int EditMDParser::TextCb(MD_TEXTTYPE nType, const MD_CHAR* pText, MD_SIZE nSize, void* pUserData)
{
    static_cast<EditMDParser*>(pUserData)->HandleText(nType, pText, nSize);
    return 0;
}

void EditMDParser::EnterBlock(MD_BLOCKTYPE nType, void* /*pDetail*/)
{
    switch (nType)
    {
        case MD_BLOCK_DOC:
            break;

        case MD_BLOCK_P:
        case MD_BLOCK_H:
        case MD_BLOCK_CODE:
        case MD_BLOCK_QUOTE:
        {
            if (mbNeedParaBreak)
                InsertParaBreak();
            mbInParagraph = true;
            break;
        }

        case MD_BLOCK_UL:
        {
            ListInfo aInfo;
            aInfo.bOrdered = false;
            maListStack.push_back(aInfo);
            mnListDepth++;
            break;
        }

        case MD_BLOCK_OL:
        {
            ListInfo aInfo;
            aInfo.bOrdered = true;
            maListStack.push_back(aInfo);
            mnListDepth++;
            break;
        }

        case MD_BLOCK_LI:
        {
            if (mbNeedParaBreak)
                InsertParaBreak();
            mbInParagraph = true;

            // Set bullet/numbering for this paragraph
            if (mnListDepth >= 0 && !maListStack.empty())
            {
                sal_Int32 nPara = mpEditEngine->GetEditDoc().GetPos(maCurSel.Max().GetNode());

                // Set outline level
                SfxItemSet aItems(mpEditEngine->GetParaAttribs(nPara));
                aItems.Put(SfxInt16Item(EE_PARA_OUTLLEVEL, static_cast<sal_Int16>(mnListDepth)));

                // Set numbering rule
                SvxNumRule aRule(SvxNumRuleFlags::BULLET_REL_SIZE, 10, false);
                for (sal_uInt16 nLevel = 0;
                     nLevel < 10 && nLevel < static_cast<sal_uInt16>(maListStack.size()); nLevel++)
                {
                    SvxNumberFormat aFmt(SVX_NUM_CHAR_SPECIAL);
                    if (maListStack[nLevel].bOrdered)
                    {
                        aFmt.SetNumberingType(SVX_NUM_ARABIC);
                        aFmt.SetLabelFollowedBy(SvxNumberFormat::LabelFollowedBy::LISTTAB);
                        aFmt.SetStart(1);
                    }
                    else
                    {
                        aFmt.SetNumberingType(SVX_NUM_CHAR_SPECIAL);
                        aFmt.SetBulletChar(0x2022); // bullet
                    }
                    aFmt.SetFirstLineIndent(0);
                    aFmt.SetAbsLSpace(720 * (nLevel + 1));
                    aRule.SetLevel(nLevel, aFmt);
                }
                aItems.Put(SvxNumBulletItem(std::move(aRule), EE_PARA_NUMBULLET));
                mpEditEngine->SetParaAttribsOnly(nPara, aItems);
            }
            break;
        }

        default:
            break;
    }
}

void EditMDParser::LeaveBlock(MD_BLOCKTYPE nType, void* /*pDetail*/)
{
    switch (nType)
    {
        case MD_BLOCK_DOC:
            break;

        case MD_BLOCK_P:
        case MD_BLOCK_H:
        case MD_BLOCK_CODE:
        case MD_BLOCK_QUOTE:
        case MD_BLOCK_LI:
        {
            mbInParagraph = false;
            mbNeedParaBreak = true;
            break;
        }

        case MD_BLOCK_UL:
        case MD_BLOCK_OL:
        {
            if (!maListStack.empty())
                maListStack.pop_back();
            mnListDepth--;
            break;
        }

        default:
            break;
    }
}

void EditMDParser::EnterSpan(MD_SPANTYPE nType, void* pDetail)
{
    switch (nType)
    {
        case MD_SPAN_STRONG:
            mbBold = true;
            break;

        case MD_SPAN_EM:
            mbItalic = true;
            break;

        case MD_SPAN_DEL:
            mbStrikethrough = true;
            break;

        case MD_SPAN_CODE:
            mbCode = true;
            break;

        case MD_SPAN_A:
        {
            auto* pADetail = static_cast<MD_SPAN_A_DETAIL*>(pDetail);
            if (pADetail->href.text && pADetail->href.size > 0)
            {
                maLinkURL
                    = OUString(pADetail->href.text, pADetail->href.size, RTL_TEXTENCODING_UTF8);
            }
            mnLinkStart = maCurSel.Max().GetIndex();
            break;
        }

        default:
            break;
    }
}

void EditMDParser::LeaveSpan(MD_SPANTYPE nType, void* /*pDetail*/)
{
    switch (nType)
    {
        case MD_SPAN_STRONG:
            mbBold = false;
            break;

        case MD_SPAN_EM:
            mbItalic = false;
            break;

        case MD_SPAN_DEL:
            mbStrikethrough = false;
            break;

        case MD_SPAN_CODE:
            mbCode = false;
            break;

        case MD_SPAN_A:
        {
            if (!maLinkURL.isEmpty())
            {
                // Insert URL field covering the link text
                sal_Int32 nEnd = maCurSel.Max().GetIndex();
                OUString aRepresentation
                    = EditDoc::GetParaAsString(maCurSel.Max().GetNode(), mnLinkStart, nEnd);

                // Delete the plain text we inserted for the link
                EditPaM aStart(maCurSel.Max().GetNode(), mnLinkStart);
                EditPaM aEnd(maCurSel.Max());
                EditSelection aDelSel(aStart, aEnd);
                maCurSel = EditSelection(mpEditEngine->DeleteSelection(aDelSel));

                // Insert the URL field
                SvxURLField aURLField(maLinkURL, aRepresentation, SvxURLFormat::Repr);
                SvxFieldItem aFieldItem(aURLField, EE_FEATURE_FIELD);
                maCurSel = EditSelection(mpEditEngine->InsertField(maCurSel, aFieldItem));
            }
            maLinkURL.clear();
            mnLinkStart = -1;
            break;
        }

        default:
            break;
    }
}

void EditMDParser::HandleText(MD_TEXTTYPE nType, const MD_CHAR* pText, MD_SIZE nSize)
{
    switch (nType)
    {
        case MD_TEXT_NORMAL:
        case MD_TEXT_CODE:
        {
            OUString aText(pText, nSize, RTL_TEXTENCODING_UTF8);
            InsertText(aText);
            break;
        }

        case MD_TEXT_BR:
        case MD_TEXT_SOFTBR:
        {
            if (nType == MD_TEXT_SOFTBR)
                InsertText(u" "_ustr);
            else
                InsertParaBreak();
            break;
        }

        case MD_TEXT_ENTITY:
        {
            OString aEntity(pText, nSize);
            OUString aText;
            if (aEntity == "&amp;")
                aText = u"&"_ustr;
            else if (aEntity == "&lt;")
                aText = u"<"_ustr;
            else if (aEntity == "&gt;")
                aText = u">"_ustr;
            else if (aEntity == "&quot;")
                aText = u"\""_ustr;
            else if (aEntity == "&apos;")
                aText = u"'"_ustr;
            else if (aEntity == "&nbsp;")
                aText = u"\u00A0"_ustr;
            else
                aText = OUString(pText, nSize, RTL_TEXTENCODING_UTF8);
            InsertText(aText);
            break;
        }

        default:
            break;
    }
}

void EditMDParser::InsertText(const OUString& rText)
{
    EditPaM aStartPaM = maCurSel.Max();
    sal_Int32 nStartIndex = aStartPaM.GetIndex();

    maCurSel = EditSelection(mpEditEngine->InsertText(maCurSel, rText));

    // Apply character formatting to the just-inserted text
    if (mbBold || mbItalic || mbStrikethrough || mbCode)
    {
        EditPaM aAttrStart(maCurSel.Max().GetNode(), nStartIndex);
        EditPaM aAttrEnd(maCurSel.Max());
        EditSelection aAttrSel(aAttrStart, aAttrEnd);

        SfxItemSet aSet(mpEditEngine->GetEmptyItemSet());
        if (mbBold)
            aSet.Put(SvxWeightItem(WEIGHT_BOLD, EE_CHAR_WEIGHT));
        if (mbItalic)
            aSet.Put(SvxPostureItem(ITALIC_NORMAL, EE_CHAR_ITALIC));
        if (mbStrikethrough)
            aSet.Put(SvxCrossedOutItem(STRIKEOUT_SINGLE, EE_CHAR_STRIKEOUT));
        if (mbCode)
        {
            SvxFontItem aFont(FAMILY_MODERN, u"Liberation Mono"_ustr, u""_ustr, PITCH_FIXED,
                              RTL_TEXTENCODING_DONTKNOW, EE_CHAR_FONTINFO);
            aSet.Put(aFont);
        }

        mpEditEngine->SetAttribs(aAttrSel, aSet);
    }
}

void EditMDParser::InsertParaBreak()
{
    maCurSel = EditSelection(mpEditEngine->InsertParaBreak(maCurSel));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
