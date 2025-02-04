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

#include <DataSourceHelper.hxx>
#include <ChartModel.hxx>
#include <ChartModelHelper.hxx>
#include <DiagramHelper.hxx>
#include <DataSeriesHelper.hxx>
#include <DataSource.hxx>
#include <ControllerLockGuard.hxx>
#include <CachedDataSequence.hxx>
#include <LabeledDataSequence.hxx>
#include <unonames.hxx>

#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/chart2/data/XDataSource.hpp>
#include <com/sun/star/chart2/data/XLabeledDataSequence.hpp>

#include <com/sun/star/chart/ChartDataRowSource.hpp>
#include <com/sun/star/chart/ErrorBarStyle.hpp>
#include <tools/diagnose_ex.h>

namespace chart
{
using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;

namespace
{
void lcl_addRanges( std::vector< OUString > & rOutResult,
                    const uno::Reference< data::XLabeledDataSequence > & xLabeledSeq )
{
    if( ! xLabeledSeq.is())
        return;
    uno::Reference< data::XDataSequence > xSeq( xLabeledSeq->getLabel());
    if( xSeq.is())
        rOutResult.push_back( xSeq->getSourceRangeRepresentation());
    xSeq.set( xLabeledSeq->getValues());
    if( xSeq.is())
        rOutResult.push_back( xSeq->getSourceRangeRepresentation());
}

void lcl_addDataSourceRanges(
    std::vector< OUString > & rOutResult,
    const uno::Reference< data::XDataSource > & xDataSource )
{
    if( xDataSource.is() )
    {
        const auto aDataSequences(xDataSource->getDataSequences());
        for (const auto& rDataSequence : aDataSequences)
            lcl_addRanges(rOutResult, rDataSequence);
    }
}

void lcl_addErrorBarRanges(
    std::vector< OUString > & rOutResult,
    const uno::Reference< XDataSeries > & xDataSeries )
{
    uno::Reference< beans::XPropertySet > xSeriesProp( xDataSeries, uno::UNO_QUERY );
    if( !xSeriesProp.is())
        return;

    try
    {
        uno::Reference< beans::XPropertySet > xErrorBarProp;
        if( ( xSeriesProp->getPropertyValue( CHART_UNONAME_ERRORBAR_Y) >>= xErrorBarProp ) &&
            xErrorBarProp.is())
        {
            sal_Int32 eStyle = css::chart::ErrorBarStyle::NONE;
            if( ( xErrorBarProp->getPropertyValue( "ErrorBarStyle") >>= eStyle ) &&
                eStyle == css::chart::ErrorBarStyle::FROM_DATA )
            {
                uno::Reference< data::XDataSource > xErrorBarDataSource( xErrorBarProp, uno::UNO_QUERY );
                if( xErrorBarDataSource.is() )
                    lcl_addDataSourceRanges( rOutResult, xErrorBarDataSource );
            }
        }

        if( ( xSeriesProp->getPropertyValue(CHART_UNONAME_ERRORBAR_X) >>= xErrorBarProp ) && xErrorBarProp.is())
        {
            sal_Int32 eStyle = css::chart::ErrorBarStyle::NONE;
            if( ( xErrorBarProp->getPropertyValue("ErrorBarStyle") >>= eStyle ) &&
                eStyle == css::chart::ErrorBarStyle::FROM_DATA )
            {
                uno::Reference< data::XDataSource > xErrorBarDataSource( xErrorBarProp, uno::UNO_QUERY );
                if( xErrorBarDataSource.is() )
                    lcl_addDataSourceRanges( rOutResult, xErrorBarDataSource );
            }
        }
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
}

} // anonymous namespace

Reference< chart2::data::XDataSource > DataSourceHelper::createDataSource(
        const Sequence< Reference< chart2::data::XLabeledDataSequence > >& rSequences )
{
    return new DataSource(rSequences);
}

Reference< chart2::data::XDataSequence > DataSourceHelper::createCachedDataSequence()
{
    return new ::chart::CachedDataSequence();
}

Reference< chart2::data::XDataSequence > DataSourceHelper::createCachedDataSequence( const OUString& rSingleText )
{
    return new ::chart::CachedDataSequence( rSingleText );
}

Reference< chart2::data::XLabeledDataSequence > DataSourceHelper::createLabeledDataSequence(
        const Reference< chart2::data::XDataSequence >& xValues ,
        const Reference< chart2::data::XDataSequence >& xLabels )
{
    return new ::chart::LabeledDataSequence( xValues, xLabels );
}

Reference< chart2::data::XLabeledDataSequence > DataSourceHelper::createLabeledDataSequence(
        const Reference< chart2::data::XDataSequence >& xValues )
{
    return new ::chart::LabeledDataSequence( xValues );
}

Reference< chart2::data::XLabeledDataSequence > DataSourceHelper::createLabeledDataSequence()
{
    return new ::chart::LabeledDataSequence;
}

uno::Sequence< beans::PropertyValue > DataSourceHelper::createArguments(
                                            bool bUseColumns, bool bFirstCellAsLabel, bool bHasCategories )
{
    css::chart::ChartDataRowSource eRowSource = css::chart::ChartDataRowSource_ROWS;
    if( bUseColumns )
        eRowSource = css::chart::ChartDataRowSource_COLUMNS;

    return
    {
        { "DataRowSource", -1, uno::Any( eRowSource), beans::PropertyState_DIRECT_VALUE },
        { "FirstCellAsLabel", -1, uno::Any( bFirstCellAsLabel ), beans::PropertyState_DIRECT_VALUE },
        { "HasCategories", -1, uno::Any( bHasCategories ), beans::PropertyState_DIRECT_VALUE }
    };
}

uno::Sequence< beans::PropertyValue > DataSourceHelper::createArguments(
                                            const OUString & rRangeRepresentation,
                                            const uno::Sequence< sal_Int32 >& rSequenceMapping,
                                            bool bUseColumns, bool bFirstCellAsLabel, bool bHasCategories )
{
    uno::Sequence< beans::PropertyValue > aArguments( createArguments( bUseColumns, bFirstCellAsLabel, bHasCategories ));
    aArguments.realloc( aArguments.getLength() + 1 );
    aArguments[aArguments.getLength() - 1] =
        beans::PropertyValue( "CellRangeRepresentation"
                              , -1, uno::Any( rRangeRepresentation )
                              , beans::PropertyState_DIRECT_VALUE );
    if( rSequenceMapping.hasElements() )
    {
        aArguments.realloc( aArguments.getLength() + 1 );
        aArguments[aArguments.getLength() - 1] =
            beans::PropertyValue( "SequenceMapping"
                                , -1, uno::Any( rSequenceMapping )
                                , beans::PropertyState_DIRECT_VALUE );
    }
    return aArguments;
}

void DataSourceHelper::readArguments( const uno::Sequence< beans::PropertyValue >& rArguments
                                     , OUString & rRangeRepresentation, uno::Sequence< sal_Int32 >& rSequenceMapping
            , bool& bUseColumns, bool& bFirstCellAsLabel, bool& bHasCategories )
{
    for(const beans::PropertyValue& rProperty : rArguments)
    {
        if ( rProperty.Name == "DataRowSource" )
        {
            css::chart::ChartDataRowSource eRowSource;
            if( rProperty.Value >>= eRowSource )
                bUseColumns = (eRowSource==css::chart::ChartDataRowSource_COLUMNS);
        }
        else if ( rProperty.Name == "FirstCellAsLabel" )
        {
            rProperty.Value >>= bFirstCellAsLabel;
        }
        else if ( rProperty.Name == "HasCategories" )
        {
            rProperty.Value >>= bHasCategories;
        }
        else if ( rProperty.Name == "CellRangeRepresentation" )
        {
            rProperty.Value >>= rRangeRepresentation;
        }
        else if ( rProperty.Name == "SequenceMapping" )
        {
            rProperty.Value >>= rSequenceMapping;
        }
    }
}

uno::Reference< chart2::data::XDataSource > DataSourceHelper::pressUsedDataIntoRectangularFormat(
        const uno::Reference< chart2::XChartDocument >& xChartDoc )
{
    std::vector< Reference< chart2::data::XLabeledDataSequence > > aResultVector;

    //categories are always the first sequence
    Reference< chart2::XDiagram > xDiagram( xChartDoc->getFirstDiagram());

    Reference< chart2::data::XLabeledDataSequence > xCategories( DiagramHelper::getCategoriesFromDiagram( xDiagram ) );
    if( xCategories.is() )
        aResultVector.push_back( xCategories );

    std::vector< Reference< chart2::XDataSeries > > aSeriesVector( DiagramHelper::getDataSeriesFromDiagram( xDiagram ) );
    uno::Reference< chart2::data::XDataSource > xSeriesSource(
        DataSeriesHelper::getDataSource( comphelper::containerToSequence(aSeriesVector) ) );
    const Sequence< Reference< chart2::data::XLabeledDataSequence > > aDataSequences( xSeriesSource->getDataSequences() );

    //the first x-values is always the next sequence //todo ... other x-values get lost for old format
    Reference< chart2::data::XLabeledDataSequence > xXValues(
        DataSeriesHelper::getDataSequenceByRole( xSeriesSource, "values-x" ) );
    if( xXValues.is() )
        aResultVector.push_back( xXValues );

    //add all other sequences now without x-values
    for(  Reference< chart2::data::XLabeledDataSequence > const & labeledData : aDataSequences )
    {
        OUString aRole = DataSeriesHelper::getRole(labeledData);
        if( aRole != "values-x" )
            aResultVector.push_back( labeledData );
    }

    return new DataSource( comphelper::containerToSequence(aResultVector) );
}

uno::Sequence< OUString > DataSourceHelper::getUsedDataRanges(
    const uno::Reference< chart2::XDiagram > & xDiagram )
{
    std::vector< OUString > aResult;

    if( xDiagram.is())
    {
        uno::Reference< data::XLabeledDataSequence > xCategories( DiagramHelper::getCategoriesFromDiagram( xDiagram ) );
        if( xCategories.is() )
            lcl_addRanges( aResult, xCategories );

        std::vector< uno::Reference< XDataSeries > > aSeriesVector( DiagramHelper::getDataSeriesFromDiagram( xDiagram ) );
        for (auto const& series : aSeriesVector)
        {
            uno::Reference< data::XDataSource > xDataSource(series, uno::UNO_QUERY);
            lcl_addDataSourceRanges( aResult, xDataSource );
            lcl_addErrorBarRanges( aResult, series );
        }
    }

    return comphelper::containerToSequence( aResult );
}

uno::Sequence< OUString > DataSourceHelper::getUsedDataRanges( const uno::Reference< frame::XModel > & xChartModel )
{
    uno::Reference< XDiagram > xDiagram( ChartModelHelper::findDiagram( xChartModel ) );
    return getUsedDataRanges( xDiagram );
}

uno::Reference< chart2::data::XDataSource > DataSourceHelper::getUsedData(
    const uno::Reference< chart2::XChartDocument >& xChartDoc )
{
    return pressUsedDataIntoRectangularFormat( xChartDoc );
}

uno::Reference< chart2::data::XDataSource > DataSourceHelper::getUsedData(
    const uno::Reference< frame::XModel >& xChartModel )
{
    std::vector< uno::Reference< chart2::data::XLabeledDataSequence > > aResult;

    uno::Reference< XDiagram > xDiagram( ChartModelHelper::findDiagram( xChartModel ) );
    uno::Reference< data::XLabeledDataSequence > xCategories( DiagramHelper::getCategoriesFromDiagram( xDiagram ) );
    if( xCategories.is() )
        aResult.push_back( xCategories );

    std::vector< uno::Reference< XDataSeries > > aSeriesVector( ChartModelHelper::getDataSeries( xChartModel ) );
    for (auto const& series : aSeriesVector)
    {
        uno::Reference< data::XDataSource > xDataSource(series, uno::UNO_QUERY);
        if( !xDataSource.is() )
            continue;
        const uno::Sequence< uno::Reference< data::XLabeledDataSequence > > aDataSequences( xDataSource->getDataSequences() );
        aResult.insert( aResult.end(), aDataSequences.begin(), aDataSequences.end() );
    }

    return uno::Reference< chart2::data::XDataSource >(
        new DataSource( comphelper::containerToSequence( aResult )));
}

uno::Reference< chart2::data::XDataSource > DataSourceHelper::getUsedData(
    ChartModel& rModel )
{
    std::vector< uno::Reference< chart2::data::XLabeledDataSequence > > aResult;

    uno::Reference< XDiagram > xDiagram( rModel.getFirstDiagram() );
    uno::Reference< data::XLabeledDataSequence > xCategories( DiagramHelper::getCategoriesFromDiagram( xDiagram ) );
    if( xCategories.is() )
        aResult.push_back( xCategories );

    std::vector< uno::Reference< XDataSeries > > aSeriesVector( ChartModelHelper::getDataSeries( rModel ) );
    for (auto const& series : aSeriesVector)
    {
        uno::Reference< data::XDataSource > xDataSource(series, uno::UNO_QUERY);
        if( !xDataSource.is() )
            continue;
        const uno::Sequence< uno::Reference< data::XLabeledDataSequence > > aDataSequences( xDataSource->getDataSequences() );
        aResult.insert( aResult.end(), aDataSequences.begin(), aDataSequences.end() );
    }

    return uno::Reference< chart2::data::XDataSource >(
        new DataSource( comphelper::containerToSequence( aResult )));
}

bool DataSourceHelper::detectRangeSegmentation(
    const uno::Reference<
        frame::XModel >& xChartModel
    , OUString& rOutRangeString
    , css::uno::Sequence< sal_Int32 >& rSequenceMapping
    , bool& rOutUseColumns
    , bool& rOutFirstCellAsLabel
    , bool& rOutHasCategories )
{
    bool bSomethingDetected = false;

    uno::Reference< XChartDocument > xChartDocument( xChartModel, uno::UNO_QUERY );
    if( !xChartDocument.is() )
        return bSomethingDetected;
    uno::Reference< data::XDataProvider >  xDataProvider( xChartDocument->getDataProvider() );
    if( !xDataProvider.is() )
        return bSomethingDetected;

    try
    {
        DataSourceHelper::readArguments(
            xDataProvider->detectArguments( pressUsedDataIntoRectangularFormat( xChartDocument ) ),
            rOutRangeString, rSequenceMapping, rOutUseColumns, rOutFirstCellAsLabel, rOutHasCategories );
        bSomethingDetected = !rOutRangeString.isEmpty();

        uno::Reference< chart2::data::XLabeledDataSequence > xCategories(
                    DiagramHelper::getCategoriesFromDiagram( xChartDocument->getFirstDiagram() ));
        rOutHasCategories = xCategories.is();
    }
    catch( uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
    return bSomethingDetected;
}

bool DataSourceHelper::allArgumentsForRectRangeDetected(
    const uno::Reference< chart2::XChartDocument >& xChartDocument )
{
    bool bHasDataRowSource = false;
    bool bHasFirstCellAsLabel = false;
    bool bHasCellRangeRepresentation = false;

    uno::Reference< data::XDataProvider > xDataProvider( xChartDocument->getDataProvider() );
    if( !xDataProvider.is() )
        return false;

    try
    {
        const uno::Sequence< beans::PropertyValue > aArguments(
            xDataProvider->detectArguments( pressUsedDataIntoRectangularFormat( xChartDocument )));
        for(const beans::PropertyValue& rProperty : aArguments)
        {
            if ( rProperty.Name == "DataRowSource" )
            {
                bHasDataRowSource =
                    (rProperty.Value.hasValue() && rProperty.Value.isExtractableTo(
                        cppu::UnoType<css::chart::ChartDataRowSource>::get()));
            }
            else if ( rProperty.Name == "FirstCellAsLabel" )
            {
                bHasFirstCellAsLabel =
                    (rProperty.Value.hasValue() && rProperty.Value.isExtractableTo(cppu::UnoType<bool>::get()));
            }
            else if ( rProperty.Name == "CellRangeRepresentation" )
            {
                OUString aRange;
                bHasCellRangeRepresentation =
                    (rProperty.Value.hasValue() && (rProperty.Value >>= aRange) && !aRange.isEmpty());
            }
        }
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }

    return (bHasCellRangeRepresentation && bHasDataRowSource && bHasFirstCellAsLabel);
}

void DataSourceHelper::setRangeSegmentation(
            const uno::Reference< frame::XModel >& xChartModel
            , const css::uno::Sequence< sal_Int32 >& rSequenceMapping
            , bool bUseColumns , bool bFirstCellAsLabel, bool bUseCategories )
{
    uno::Reference< XChartDocument > xChartDocument( xChartModel, uno::UNO_QUERY );
    if( !xChartDocument.is() )
        return;
    uno::Reference< data::XDataProvider > xDataProvider( xChartDocument->getDataProvider() );
    if( !xDataProvider.is() )
        return;
    uno::Reference< XDiagram > xDiagram( ChartModelHelper::findDiagram( xChartModel ) );
    if( !xDiagram.is() )
        return;
    uno::Reference< chart2::XChartTypeManager > xChartTypeManager( xChartDocument->getChartTypeManager() );
    if( !xChartTypeManager.is() )
        return;
    uno::Reference< lang::XMultiServiceFactory > xTemplateFactory( xChartTypeManager, uno::UNO_QUERY );
    if( !xTemplateFactory.is() )
        return;

    OUString aRangeString;
    bool bDummy;
    uno::Sequence< sal_Int32 > aDummy;
    readArguments( xDataProvider->detectArguments( pressUsedDataIntoRectangularFormat( xChartDocument )),
                   aRangeString, aDummy, bDummy, bDummy, bDummy );

    uno::Sequence< beans::PropertyValue > aArguments(
        createArguments( aRangeString, rSequenceMapping, bUseColumns, bFirstCellAsLabel, bUseCategories ) );

    uno::Reference< chart2::data::XDataSource > xDataSource( xDataProvider->createDataSource(
                                                                 aArguments ) );
    if( !xDataSource.is() )
        return;

    ControllerLockGuardUNO aCtrlLockGuard( xChartModel );
    xDiagram->setDiagramData( xDataSource, aArguments );
}

Sequence< OUString > DataSourceHelper::getRangesFromLabeledDataSequence(
    const Reference< data::XLabeledDataSequence > & xLSeq )
{
    Sequence< OUString > aResult;
    if( xLSeq.is())
    {
        Reference< data::XDataSequence > xLabel( xLSeq->getLabel());
        Reference< data::XDataSequence > xValues( xLSeq->getValues());

        if( xLabel.is())
        {
            if( xValues.is())
            {
                aResult.realloc( 2 );
                aResult[0] = xLabel->getSourceRangeRepresentation();
                aResult[1] = xValues->getSourceRangeRepresentation();
            }
            else
            {
                aResult.realloc( 1 );
                aResult[0] = xLabel->getSourceRangeRepresentation();
            }
        }
        else if( xValues.is())
        {
            aResult.realloc( 1 );
            aResult[0] = xValues->getSourceRangeRepresentation();
        }
    }
    return aResult;
}

OUString DataSourceHelper::getRangeFromValues(
    const Reference< data::XLabeledDataSequence > & xLSeq )
{
    OUString aResult;
    if( xLSeq.is() )
    {
        Reference< data::XDataSequence > xValues( xLSeq->getValues() );
        if( xValues.is() )
            aResult = xValues->getSourceRangeRepresentation();
    }
    return aResult;
}

Sequence< OUString > DataSourceHelper::getRangesFromDataSource( const Reference< data::XDataSource > & xSource )
{
    std::vector< OUString > aResult;
    if( xSource.is())
    {
        const Sequence< Reference< data::XLabeledDataSequence > > aLSeqSeq( xSource->getDataSequences());
        for( Reference< data::XLabeledDataSequence > const & labeledData : aLSeqSeq )
        {
            Reference< data::XDataSequence > xLabel( labeledData->getLabel());
            Reference< data::XDataSequence > xValues( labeledData->getValues());

            if( xLabel.is())
                aResult.push_back( xLabel->getSourceRangeRepresentation());
            if( xValues.is())
                aResult.push_back( xValues->getSourceRangeRepresentation());
        }
    }
    return comphelper::containerToSequence( aResult );
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
