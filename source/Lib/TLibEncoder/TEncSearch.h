/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncSearch.h
    \brief    encoder search class (header)
*/

#ifndef __TENCSEARCH__
#define __TENCSEARCH__

// Include files
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TLibCommon/TComPattern.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComRectangle.h"
#include "TEncEntropy.h"
#include "TEncSbac.h"
#include "TEncCfg.h"
#include "TLibCommon/TComHash.h"

// ====================================================================================================================
// Constants 
// ====================================================================================================================

#define CHROMA_REFINEMENT_CANDIDATES  8

//! \ingroup TLibEncoder
//! \{

class TEncCu;

#define INTRABC_HASH_DEPTH                     1  ////< Currently used only for 8x8
#define INTRABC_HASH_TABLESIZE                (1 << 16)

struct IntraBCHashNode
{
  Int pos_X;
  Int pos_Y;
  IntraBCHashNode * next;
};

// ====================================================================================================================
// Class definition
// ====================================================================================================================

static const UInt MAX_NUM_REF_LIST_ADAPT_SR=2;
static const UInt MAX_IDX_ADAPT_SR=33;
static const UInt NUM_MV_PREDICTORS=3;

/// encoder search class
class TEncSearch : public TComPrediction
{
private:
  TCoeff**        m_ppcQTTempCoeff[MAX_NUM_COMPONENT /* 0->Y, 1->Cb, 2->Cr*/];
  TCoeff*         m_pcQTTempCoeff[MAX_NUM_COMPONENT];
#if ADAPTIVE_QP_SELECTION
  TCoeff**        m_ppcQTTempArlCoeff[MAX_NUM_COMPONENT];
  TCoeff*         m_pcQTTempArlCoeff[MAX_NUM_COMPONENT];
#endif
  UChar*          m_puhQTTempTrIdx;
  UChar*          m_puhQTTempCbf[MAX_NUM_COMPONENT];

  TComYuv*        m_pcQTTempTComYuv;
  TComYuv         m_tmpYuvPred; // To be used in xGetInterPredictionError() to avoid constant memory allocation/deallocation

  Char*           m_phQTTempCrossComponentPredictionAlpha[MAX_NUM_COMPONENT];
  Pel*            m_pSharedPredTransformSkip[MAX_NUM_COMPONENT];
  TCoeff*         m_pcQTTempTUCoeff[MAX_NUM_COMPONENT];
  UChar*          m_puhQTTempTransformSkipFlag[MAX_NUM_COMPONENT];
  TComYuv         m_pcQTTempTransformSkipTComYuv;
#if ADAPTIVE_QP_SELECTION
  TCoeff*         m_ppcQTTempTUArlCoeff[MAX_NUM_COMPONENT];
#endif

  IntraBCHashNode***      m_pcIntraBCHashTable;                 ///< The hash table used for Intra BC search
#if SCM_U0106_ACT_TU_SIG
  Bool*           m_puhQTTempACTFlag;
  TCoeff*         m_pcACTTempTUCoeff[MAX_NUM_COMPONENT];
  TComYuv         m_pcACTTempTransformSkipTComYuv;
  TComYuv*        m_pcQTTempTComYuvCS;
  TComYuv*        m_pcNoCorrYuvTmp;
#if ADAPTIVE_QP_SELECTION
  TCoeff*         m_ppcACTTempTUArlCoeff[MAX_NUM_COMPONENT];
#endif
  TComACTTURDCost m_sACTRDCostTU[5];
#endif

protected:
  // interface to option
  TEncCfg*        m_pcEncCfg;

  // interface to classes
  TComTrQuant*    m_pcTrQuant;
  TComRdCost*     m_pcRdCost;
  TEncEntropy*    m_pcEntropyCoder;

  // ME parameters
  Int             m_iSearchRange;
  Int             m_bipredSearchRange; // Search range for bi-prediction
  Int             m_iFastSearch;
  Int             m_aaiAdaptSR[MAX_NUM_REF_LIST_ADAPT_SR][MAX_IDX_ADAPT_SR];
  TComMv          m_cSrchRngLT;
  TComMv          m_cSrchRngRB;
  TComMv          m_acMvPredictors[NUM_MV_PREDICTORS]; // Left, Above, AboveRight. enum MVP_DIR first NUM_MV_PREDICTORS entries are suitable for accessing.

  // RD computation
  TEncSbac***     m_pppcRDSbacCoder;
  TEncSbac*       m_pcRDGoOnSbacCoder;
  DistParam       m_cDistParam;

  // Misc.
  Pel*            m_pTempPel;
  const UInt*     m_puiDFilter;

  // AMVP cost computation
  // UInt            m_auiMVPIdxCost[AMVP_MAX_NUM_CANDS+1][AMVP_MAX_NUM_CANDS];
  UInt            m_auiMVPIdxCost[AMVP_MAX_NUM_CANDS+1][AMVP_MAX_NUM_CANDS+1]; //th array bounds

  RefPicList      m_currRefPicList;
  Int             m_currRefPicIndex;
  Bool            m_bSkipFracME;
  TComMv          m_integerMv2Nx2N[NUM_REF_PIC_LIST_01][MAX_NUM_REF];
  TComMv          m_acBVs[SCM_S0067_NUM_CANDIDATES];
  UInt            m_uiNumBVs, m_uiNumBV16s;
  Distortion      m_lastCandCost;
  Bool            m_bBestScanRotationMode;
  Pel*            m_paOriginalLevel;
  Pel*            m_paBestLevel[MAX_NUM_COMPONENT];
  UChar*          m_paBestSPoint;
  TCoeff*         m_paBestRun;
  UChar*          m_paBestEscapeFlag;
#if SCM_U0096_PLT_ENCODER_IMPROVEMENT
  UInt            m_prevPltSize[MAX_PLT_ITER];
  Pel             m_prevPlt[MAX_PLT_ITER][3][MAX_PLT_SIZE];
  UInt            m_forcePltSize;
  Pel             m_forcePlt[3][MAX_PLT_SIZE];

  Pel*            m_paLevelStoreRD[MAX_NUM_COMPONENT];
  UChar*          m_paSPointStoreRD;
  TCoeff*         m_paRunStoreRD;
  UChar*          m_paEscapeFlagStoreRD;

  UInt m_runGolombGroups[32*32];

  pltInfoStruct m_currentPLTElement;
  pltInfoStruct m_nextPLTElement;
#endif
  Bool          m_isInitialized;
public:
  TEncSearch();
  virtual ~TEncSearch();

  TComYuv* getTmpYuvPred() {return &m_tmpYuvPred;}

  Void init(TEncCfg*      pcEncCfg,
            TComTrQuant*  pcTrQuant,
            Int           iSearchRange,
            Int           bipredSearchRange,
            Int           iFastSearch,
            const UInt    maxCUWidth,
            const UInt    maxCUHeight,
            const UInt    maxTotalCUDepth,
            TEncEntropy*  pcEntropyCoder,
            TComRdCost*   pcRdCost,
            TEncSbac***   pppcRDSbacCoder,
            TEncSbac*     pcRDGoOnSbacCoder );

  Void destroy();

protected:

  /// sub-function for motion vector refinement used in fractional-pel accuracy
  Distortion  xPatternRefinement( TComPattern* pcPatternKey,
                                  TComMv baseRefMv,
                                  Int iFrac, TComMv& rcMvFrac, Bool bAllowUseOfHadamard
                                 );

  typedef struct
  {
    Pel*        piRefY;
    Int         iYStride;
    Int         iBestX;
    Int         iBestY;
    UInt        uiBestRound;
    UInt        uiBestDistance;
    Distortion  uiBestSad;
    UChar       ucPointNr;
  } IntTZSearchStruct;

  // sub-functions for ME
  __inline Void xTZSearchHelp         ( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance );
  __inline Void xTZ2PointSearch       ( TComPattern* pcPatternKey, IntTZSearchStruct& rcStrukt, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB );
  __inline Void xTZ8PointSquareSearch ( TComPattern* pcPatternKey, IntTZSearchStruct& rcStrukt, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist );
  __inline Void xTZ8PointDiamondSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStrukt, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist, Bool bSkipLeftDist2 = false, Bool bSkipTopDist2 = false );

  Void xGetInterPredictionError( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, Distortion& ruiSAD, Bool Hadamard );

public:
  Void  estIntraPredLumaQT      ( TComDataCU* pcCU,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*    pcPredYuv,
                                  TComYuv*    pcResiYuv,
                                  TComYuv*    pcRecoYuv,
                                  Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE]
                                  DEBUG_STRING_FN_DECLARE(sDebug));

  Void  estIntraPredChromaQT    ( TComDataCU* pcCU,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*    pcPredYuv,
                                  TComYuv*    pcResiYuv,
                                  TComYuv*    pcRecoYuv,
                                  Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE]
                                  DEBUG_STRING_FN_DECLARE(sDebug));

#if SCM_U0106_ACT_TU_SIG
  Void  estIntraPredQTCT        ( TComDataCU*    pcCU,
                                  TComYuv*       pcOrgYuv,
                                  TComYuv*       pcPredYuv,
                                  TComYuv*       pcResiYuv,
                                  TComYuv*       pcRecoYuv,
                                  ACTRDTestTypes eACTRDTestType,
                                  Bool           bReuseIntraMode
                                  DEBUG_STRING_FN_DECLARE(sDebug)
                                 );
#else
  Void  estIntraPredQTCT        ( TComDataCU* pcCU,
                                   TComYuv*    pcOrgYuv,
                                   TComYuv*    pcPredYuv,
                                   TComYuv*    pcResiYuv,
                                   TComYuv*    pcRecoYuv
                                   DEBUG_STRING_FN_DECLARE(sDebug)
                                 );
#endif

  Void  estIntraPredLumaQTWithModeReuse ( TComDataCU* pcCU,
                                          TComYuv*    pcOrgYuv,
                                          TComYuv*    pcPredYuv,
                                          TComYuv*    pcResiYuv,
                                          TComYuv*    pcRecoYuv,
                                          Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE]
                                    );

  Void  estIntraPredChromaQTWithModeReuse ( TComDataCU* pcCU,
                                            TComYuv*    pcOrgYuv,
                                            TComYuv*    pcPredYuv,
                                            TComYuv*    pcResiYuv,
                                            TComYuv*    pcRecoYuv,
                                            Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE]
                                          );

  /// encoder estimation - inter prediction (non-skip)
  Void predInterSearch          ( TComDataCU* pcCU,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*    pcPredYuv,
                                  TComYuv*    pcResiYuv,
                                  TComYuv*    pcRecoYuv
                                  DEBUG_STRING_FN_DECLARE(sDebug),
                                  Bool        bUseRes = false
#if AMP_MRG
                                 ,Bool        bUseMRG = false
#endif
                                , TComMv*     iMVCandList = NULL
                                );

  Bool isBlockVectorValid( Int xPos, Int yPos, Int width, Int height, TComDataCU *pcCU, UInt uiAbsPartIdx,
                           Int xStartInCU, Int yStartInCU, Int xBv, Int yBv, Int ctuSize );

  Bool predIntraBCSearch        ( TComDataCU* pcCU,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*&   rpcPredYuv,
                                  TComYuv*&   rpcResiYuv,
                                  TComYuv*&   rpcRecoYuv
                                  DEBUG_STRING_FN_DECLARE(sDebug),
                                  Bool        bUse1DSearchFor8x8,
                                  Bool        bUseRes
                                , Bool        testOnlyPred
                                );

  Bool predMixedIntraBCInterSearch( TComDataCU* pcCU,
                                    TComYuv*    pcOrgYuv,
                                    TComYuv*&   rpcPredYuv,
                                    TComYuv*&   rpcResiYuv,
                                    TComYuv*&   rpcRecoYuv
                                    DEBUG_STRING_FN_DECLARE( sDebug ),
                                    TComMv*     iMVCandList,
                                    Bool        bUseRes
                                    );

  Void xIntraBlockCopyEstimation( TComDataCU*  pcCU,
                                  TComYuv*     pcYuvOrg,
                                  Int          iPartIdx,
                                  TComMv*      pcMvPred,
                                  TComMv&      rcMv,
                                  Distortion&  ruiCost,
                                  Bool         bUse1DSearchFor8x8
                                , Bool         testOnlyPred
                                );

  Void addToSortList            ( list<BlockHash>& listBlockHash,
                                  list<Int>& listCost,
                                  Int cost,
                                  const BlockHash& blockHash
                                );

  Distortion getSAD             ( Pel* pRef,
                                  Int refStride,
                                  Pel* pCurr,
                                  Int currStride,
                                  Int width,
                                  Int height,
                                  const BitDepths& bitDepths
                                );

  Bool predInterHashSearch      ( TComDataCU* pcCU,
                                  TComYuv* pcOrg,
                                  TComYuv*& rpcPredYuv,
                                  Bool& isPerfectMatch
                                );

  Bool xHashInterEstimation     ( TComDataCU* pcCU,
                                  Int width,
                                  Int height,
                                  RefPicList& bestRefPicList,
                                  Int& bestRefIndex,
                                  TComMv& bestMv,
                                  TComMv& bestMvd,
                                  Int& bestMVPIndex,
                                  Bool& isPerfectMatch
                                 );
  
  Int  xHashInterPredME         ( TComDataCU* pcCU,
                                  Int width,
                                  Int height,
                                  RefPicList currRefPicList,
                                  Int currRefPicIndex,
                                  TComMv bestMv[5]
                                );

  Void selectMatchesInter       ( TComDataCU* pcCU,
                                  const MapIterator& itBegin,
                                  Int count,
                                  list<BlockHash>& vecBlockHash,
                                  const BlockHash& currBlockHash
                                 );

  Void xSetIntraSearchRange     ( TComDataCU*   pcCU,
                                  TComMv&       cMvPred,
                                  UInt          uiPartAddr,
                                  Int           iRoiWidth,
                                  Int           iRoiHeight,
                                  TComMv&       rcMvSrchRngLT,
                                  TComMv&       rcMvSrchRngRB );

#if SCM_FIX_TICKET_1401
  Bool xCIPIBCSearchPruning(    TComDataCU*   pcCU,
                                Int           refPixlX,
                                Int           refPixlY,
                                Int           roiWidth,
                                Int           roiHeight);
#else
  Bool xCIPIntraSearchPruning(    TComDataCU*   pcCU,
                                  Int           relX,
                                  Int           relY,
                                  Int           roiWidth,
                                  Int           roiHeight);
#endif

#if SCM_FIX_TICKET_1401
  Bool isValidIntraBCSearchArea(  TComDataCU*   pcCU,
                                  Int           predX,
                                  Int           predY,
                                  Int           roiWidth,
                                  Int           roiHeight,
                                  Int           uiPartOffset)
#else
  Bool isValidIntraBCSearchArea(  TComDataCU*   pcCU,
                                  Int           iPartIdx,
                                  Int           predX,
                                  Int           ROIStartX,
                                  Int           predY,
                                  Int           ROIStartY,
                                  Int           roiWidth,
                                  Int           roiHeight,
                                  Int           uiPartOffset)
#endif
  {
    const Int uiMaxCuWidth   = pcCU->getSlice()->getSPS()->getMaxCUWidth();
    const Int uiMaxCuHeight  = pcCU->getSlice()->getSPS()->getMaxCUHeight();
    const Int  cuPelX        = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiPartOffset ] ];
    const Int  cuPelY        = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiPartOffset ] ];
    Int uiRefCuX             = (cuPelX + predX + roiWidth  - 1)/uiMaxCuWidth;
    Int uiRefCuY             = (cuPelY + predY + roiHeight - 1)/uiMaxCuHeight;
    Int uiCuPelX             = (cuPelX / uiMaxCuWidth);
    Int uiCuPelY             = (cuPelY / uiMaxCuHeight);
    
    if(((Int)(uiRefCuX - uiCuPelX) > (Int)((uiCuPelY - uiRefCuY))))
    {
      return false;
    }

#if SCM_T0056_IBC_VALIDATE_TILES
    const UInt curTileIdx = pcCU->getPic()->getPicSym()->getTileIdxMap( pcCU->getCtuRsAddr() );
    TComTile* curTile = pcCU->getPic()->getPicSym()->getTComTile( curTileIdx );

    const Int tileAreaRight  = (curTile->getRightEdgePosInCtus() + 1) * uiMaxCuWidth;
    const Int tileAreaBottom = (curTile->getBottomEdgePosInCtus() + 1) * uiMaxCuHeight;

    const Int tileAreaLeft   = tileAreaRight - curTile->getTileWidthInCtus() * uiMaxCuWidth;
    const Int tileAreaTop    = tileAreaBottom - curTile->getTileHeightInCtus() * uiMaxCuHeight;

    if( (cuPelX + predX + roiWidth > tileAreaRight) || (cuPelY + predY + roiHeight > tileAreaBottom) ||
      (cuPelX + predX < tileAreaLeft) || (cuPelY + predY < tileAreaTop) )        
    {
      return false;
    }
#endif

    TComSlice *pcSlice = pcCU->getSlice();
    if( pcSlice->getSliceMode() )
    {
      TComPicSym *pcSym = pcCU->getPic()->getPicSym();
      Int      ctuX = (cuPelX + predX) / uiMaxCuWidth;
      Int      ctuY = (cuPelY + predY) / uiMaxCuHeight;
      UInt   refCtu = ctuX + pcSym->getFrameWidthInCtus()*ctuY;
      UInt startCtu = pcSym->getCtuTsToRsAddrMap( pcCU->getSlice()->getSliceSegmentCurStartCtuTsAddr() );
      if (refCtu < startCtu) return false;
    }

#if SCM_FIX_TICKET_1401
    return (!pcCU->getSlice()->getPPS()->getConstrainedIntraPred()) ||
            xCIPIBCSearchPruning(pcCU, cuPelX + predX, cuPelY + predY, roiWidth, roiHeight);
#else
    // check boundary
    if ( pcCU->getWidth( 0 ) == 8 && pcCU->getPartitionSize( 0 ) != SIZE_2Nx2N && pcCU->getSlice()->getPic()->getPicYuvOrg()->getChromaFormat() != CHROMA_444 )
    {
      if ( pcCU->getSlice()->getPic()->getPicYuvOrg()->getChromaFormat() == CHROMA_420 )
      {
        if ( (pcCU->getPartitionSize( 0 ) == SIZE_NxN && iPartIdx == 3) ||
             (pcCU->getPartitionSize( 0 ) == SIZE_2NxN && iPartIdx == 1) ||
             (pcCU->getPartitionSize( 0 ) == SIZE_Nx2N && iPartIdx == 1) )
        {
          Int cuStartX = pcCU->getCUPelX();
          Int cuStartY = pcCU->getCUPelY();
          Int refStartX = cuStartX + predX;
          Int refStartY = cuStartY + predY;
          Int refEndX = refStartX + 8 - 1;
          Int refEndY = refStartY + 8 - 1;
          if ( refStartX < 0 || refStartY < 0 ||
               refEndX >= pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ||
               refEndY >= pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() )
          {
            return false;
          }
        }
      }
      else if ( pcCU->getSlice()->getPic()->getPicYuvOrg()->getChromaFormat() == CHROMA_422 )
      {
        if ( pcCU->getPartitionSize( 0 ) == SIZE_Nx2N && iPartIdx == 1 )
        {
          Int cuStartX = pcCU->getCUPelX();
          Int cuStartY = pcCU->getCUPelY();
          Int refStartX = cuStartX + predX;
          Int refStartY = cuStartY + predY;
          Int refEndX = refStartX + 8 - 1;
          Int refEndY = refStartY + 8 - 1;
          if ( refStartX < 0 || refStartY < 0 ||
               refEndX >= pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ||
               refEndY >= pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() )
          {
            return false;
          }
        }
        else if ( pcCU->getPartitionSize( 0 ) == SIZE_NxN && iPartIdx == 1 )
        {
          Int cuStartX = pcCU->getCUPelX();
          Int cuStartY = pcCU->getCUPelY();
          Int refStartX = cuStartX + predX;
          Int refStartY = cuStartY + predY;
          Int refEndX = refStartX + 8 - 1;
          Int refEndY = refStartY + 4 - 1;
          if ( refStartX < 0 || refStartY < 0 ||
               refEndX >= pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ||
               refEndY >= pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() )
          {
            return false;
          }
        }
        else if ( pcCU->getPartitionSize( 0 ) == SIZE_NxN && iPartIdx == 3 )
        {
          Int cuStartX = pcCU->getCUPelX();
          Int cuStartY = pcCU->getCUPelY();
          Int refStartX = cuStartX + predX;
          Int refStartY = cuStartY + 4 + predY;
          Int refEndX = refStartX + 8 - 1;
          Int refEndY = refStartY + 4 - 1;
          if ( refStartX < 0 || refStartY < 0 ||
               refEndX >= pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ||
               refEndY >= pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() )
          {
            return false;
          }
        }
      }
    }

    return (!pcCU->getSlice()->getPPS()->getConstrainedIntraPred())        ||
           (pcCU->getSlice()->getSliceType() == I_SLICE)                   ||
           xCIPIntraSearchPruning(pcCU, predX + ROIStartX, predY + ROIStartY, roiWidth, roiHeight);
#endif
  }

  Void xIntraBCSearchMVCandUpdate(Distortion uiSad, Int x, Int y, Distortion* uiSadBestCand, TComMv* cMVCand);
  
  Int xIntraBCSearchMVChromaRefine( TComDataCU *pcCU,
                                    Int         iRoiWidth,
                                    Int         iRoiHeight,
                                    Int         cuPelX,
                                    Int         cuPelY,
                                    Distortion* uiSadBestCand, 
                                    TComMv*     cMVCand, 
                                    UInt        uiPartAddr);

  Void xIntraPatternSearch      ( TComDataCU*  pcCU,
                                  Int          iPartIdx,
                                  UInt         uiPartAddr,
                                  TComPattern* pcPatternKey,
                                  Pel*         piRefY,
                                  Int          iRefStride,
                                  TComMv*      pcMvSrchRngLT,
                                  TComMv*      pcMvSrchRngRB,
                                  TComMv&      rcMv,
                                  Distortion&  ruiSAD,
                                  Int          iRoiWidth,
                                  Int          iRoiHeight,
                                  TComMv*      mvPreds, 
                                  Bool         bUse1DSearchFor8x8
                                , Bool         testOnlyPred
                                );

  /// encode residual and compute rd-cost for inter mode
#if SCM_U0106_ACT_TU_SIG
  Void encodeResAndCalcRdInterCU( TComDataCU* pcCU,
                                  TComYuv*    pcYuvOrg,
                                  TComYuv*    pcYuvPred,
                                  TComYuv*    pcYuvResi,
                                  TComYuv*    pcYuvResiBest,
                                  TComYuv*    pcYuvRec,
                                  Bool        bSkipResidual,
                                  TComYuv*    pcYuvNoCorrResi,
                                  ACTRDTestTypes eACTRDTestType
                                  DEBUG_STRING_FN_DECLARE(sDebug) );
#else
  Void encodeResAndCalcRdInterCU( TComDataCU* pcCU,
                                  TComYuv*    pcYuvOrg,
                                  TComYuv*    pcYuvPred,
                                  TComYuv*    pcYuvResi,
                                  TComYuv*    pcYuvResiBest,
                                  TComYuv*    pcYuvRec,
                                  Bool        bSkipResidual,
                                  TComYuv*    pcYuvNoCorrResi
                                  DEBUG_STRING_FN_DECLARE(sDebug) );
#endif

  /// set ME search range
  Void setAdaptiveSearchRange   ( Int iDir, Int iRefIdx, Int iSearchRange) { assert(iDir < MAX_NUM_REF_LIST_ADAPT_SR && iRefIdx<Int(MAX_IDX_ADAPT_SR)); m_aaiAdaptSR[iDir][iRefIdx] = iSearchRange; }

  Void xEncPCM    (TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piOrg, Pel* piPCM, Pel* piPred, Pel* piResi, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, const ComponentID compID );
  Void IPCMSearch (TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* rpcPredYuv, TComYuv* rpcResiYuv, TComYuv* rpcRecoYuv );
#if SCM_U0096_PLT_ENCODER_IMPROVEMENT
  UInt PLTSearch  (TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv,TComYuv *& rpcResiBestYuv, TComYuv*& rpcRecoYuv, Bool forcePLTPrediction,
     UInt uiIterNumber, UInt *pltSize);
#else
  Void PLTSearch  (TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv,TComYuv *& rpcResiBestYuv, TComYuv*& rpcRecoYuv, Bool forcePLTPrediction);
#endif

  Void deriveRunAndCalcBits( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* pcRecoYuv, UInt& uiMinBits, Bool bReset, PLTScanMode pltScanMode);

  Int xIntraBCHashTableIndex  ( TComDataCU* pcCU,
                                Int pos_X,
                                Int pos_Y,
                                Int width,
                                Int height,
                                Bool isRec
                              );

  Void xIntraBCHashSearch     ( TComDataCU* pcCU,
                                TComYuv* pcYuvOrg,
                                Int iPartIdx,
                                TComMv* pcMvPred,
                                TComMv& rcMv,
                                UInt uiIntraBCECost
                              );

  Void xIntraBCHashTableUpdate( TComDataCU* pcCU,
                                Bool isRec
                              );

  Void xClearIntraBCHashTable();

  Void setHashLinklist        ( IntraBCHashNode*& HashLinklist,
                                UInt uiDepth,
                                UInt uiHashIdx
                              );

  IntraBCHashNode* getHashLinklist( UInt uiDepth, Int iHashIdx ) { return m_pcIntraBCHashTable[uiDepth][iHashIdx]; }


protected:

  // -------------------------------------------------------------------------------------------------------------------
  // Intra search
  // -------------------------------------------------------------------------------------------------------------------

  Void  xEncSubdivCbfQT           ( TComTU      &rTu,
                                    Bool         bLuma,
                                    Bool         bChroma );

  Void  xEncCoeffQT               ( TComTU &rTu,
                                    ComponentID  component,
                                    Bool         bRealCoeff );
#if SCM_U0106_ACT_TU_SIG
  Void  xEncColorTransformFlagQT  ( TComTU &rTu );
#endif

  Void  xEncIntraHeader           ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    Bool         bLuma,
                                    Bool         bChroma );
  UInt  xGetIntraBitsQT           ( TComTU &rTu,
                                    Bool         bLuma,
                                    Bool         bChroma,
                                    Bool         bRealCoeff );

  UInt  xGetIntraBitsQTChroma    ( TComTU &rTu,
                                   ComponentID compID,
                                   Bool          bRealCoeff );

  Void  xIntraCodingTUBlock       (       TComYuv*      pcOrgYuv,
                                          TComYuv*      pcPredYuv,
                                          TComYuv*      pcResiYuv,
                                          Pel           resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE],
                                    const Bool          checkCrossCPrediction,
                                          Distortion&   ruiDist,
                                    const ComponentID   compID,
                                          TComTU        &rTu
                                    DEBUG_STRING_FN_DECLARE(sTest)
                                         ,Int           default0Save1Load2 = 0
                                   );

  Void  xIntraCodingTUBlockCSC    (       TComYuv*      pcResiYuv,
                                          Pel           resiLuma[MAX_CU_SIZE * MAX_CU_SIZE],
                                    const Bool          checkDecorrelation,
                                    const ComponentID   compID,
                                          TComTU        &rTu,
                                          QpParam       &cQP
                                          DEBUG_STRING_FN_DECLARE(sDebug)
                                  );

  Void  xRecurIntraCodingLumaQT   ( TComYuv*    pcOrgYuv,
                                    TComYuv*    pcPredYuv,
                                    TComYuv*    pcResiYuv,
                                    Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE],
                                    Distortion& ruiDistY,
#if HHI_RQT_INTRA_SPEEDUP
                                    Bool         bCheckFirst,
#endif
                                    Double&      dRDCost,
                                    TComTU      &rTu
                                    DEBUG_STRING_FN_DECLARE(sDebug));

  Void  xSetIntraResultLumaQT     ( TComYuv*     pcRecoYuv,
                                    TComTU &rTu);

  Void xStoreCrossComponentPredictionResult  (       Pel    *pResiLuma,
                                               const Pel    *pBestLuma,
                                                     TComTU &rTu,
                                               const Int     xOffset,
                                               const Int     yOffset,
                                               const Int     strideResi,
                                               const Int     strideBest );

  Char xCalcCrossComponentPredictionAlpha    (       TComTU &rTu,
                                               const ComponentID compID,
                                               const Pel*        piResiL,
                                               const Pel*        piResiC,
                                               const Int         width,
                                               const Int         height,
                                               const Int         strideL,
                                               const Int         strideC );

  Void  xRecurIntraChromaCodingQT ( TComYuv*    pcOrgYuv,
                                    TComYuv*    pcPredYuv,
                                    TComYuv*    pcResiYuv,
                                    Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE],
                                    Distortion& ruiDist,
                                    TComTU      &rTu
                                    DEBUG_STRING_FN_DECLARE(sDebug));

#if SCM_U0106_ACT_TU_SIG
  Void  xRecurIntraCodingQTTUCSC  ( TComYuv*       pcOrgYuv,
                                    TComYuv*       pcPredYuv,
                                    TComYuv*       pcResiYuv,
                                    Distortion&    uiPUDistY,
                                    Distortion&    uiPUDistC,
                                    Double&        dPUCost,
                                    TComTU&        rTu,
                                    Bool           bTestMaxTUSize,
                                    ACTRDTestTypes eACTRDTestType
                                    DEBUG_STRING_FN_DECLARE(sDebug)
                                   );
#endif

  Void  xRecurIntraCodingQTCSC    ( TComYuv*     pcOrgYuv,
                                    TComYuv*     pcPredYuv,
                                    TComYuv*     pcResiYuv,
                                    Distortion&  uiPUDistY,
                                    Distortion&  uiPUDistC,
                                    Double&      dPUCost,
                                    TComTU&      rTu,
                                    Bool         bTestMaxTUSize
                                    DEBUG_STRING_FN_DECLARE(sDebug)
                                  );

  Void  xSetIntraResultChromaQT   ( TComYuv*    pcRecoYuv, TComTU &rTu);

#if SCM_U0106_ACT_TU_SIG
  Void  xStoreIntraResultQT       ( const ComponentID compID, TComTU &rTu, Bool bACTCache = false);
  Void  xLoadIntraResultQT        ( const ComponentID compID, TComTU &rTu, Bool bACTCache = false);
#else
  Void  xStoreIntraResultQT       ( const ComponentID compID, TComTU &rTu);
  Void  xLoadIntraResultQT        ( const ComponentID compID, TComTU &rTu);
#endif


  // -------------------------------------------------------------------------------------------------------------------
  // Inter search (AMP)
  // -------------------------------------------------------------------------------------------------------------------

  Void xEstimateMvPredAMVP        ( TComDataCU* pcCU,
                                    TComYuv*    pcOrgYuv,
                                    UInt        uiPartIdx,
                                    RefPicList  eRefPicList,
                                    Int         iRefIdx,
                                    TComMv&     rcMvPred,
                                    Bool        bFilled = false
                                  , Distortion* puiDistBiP = NULL
                                     );

  Void xCheckBestMVP              ( TComDataCU* pcCU,
                                    RefPicList  eRefPicList,
                                    TComMv      cMv,
                                    TComMv&     rcMvPred,
                                    Int&        riMVPIdx,
                                    UInt&       ruiBits,
                                    Distortion& ruiCost );

  Distortion xGetTemplateCost    ( TComDataCU*  pcCU,
                                    UInt        uiPartAddr,
                                    TComYuv*    pcOrgYuv,
                                    TComYuv*    pcTemplateCand,
                                    TComMv      cMvCand,
                                    Int         iMVPIdx,
                                    Int         iMVPNum,
                                    RefPicList  eRefPicList,
                                    Int         iRefIdx,
                                    Int         iSizeX,
                                    Int         iSizeY
                                   );


  Void xCopyAMVPInfo              ( AMVPInfo*   pSrc, AMVPInfo* pDst );
  UInt xGetMvpIdxBits             ( Int iIdx, Int iNum );
  Void xGetBlkBits                ( PartSize  eCUMode, Bool bPSlice, Int iPartIdx,  UInt uiLastMode, UInt uiBlkBit[3]);

  Void xMergeEstimation           ( TComDataCU*  pcCU,
                                    TComYuv*     pcYuvOrg,
                                    Int          iPartIdx,
                                    UInt&        uiInterDir,
                                    TComMvField* pacMvField,
                                    UInt&        uiMergeIndex,
                                    Distortion&  ruiCost,
                                    TComMvField* cMvFieldNeighbours,
                                    UChar*       uhInterDirNeighbours,
                                    Int&         numValidMergeCand,
                                    Int          iCostCalcType = 0
                                   );

  Void xRestrictBipredMergeCand   ( TComDataCU*     pcCU,
                                    UInt            puIdx,
                                    TComMvField*    mvFieldNeighbours,
                                    UChar*          interDirNeighbours,
                                    Int             numValidMergeCand );


  // -------------------------------------------------------------------------------------------------------------------
  // motion estimation
  // -------------------------------------------------------------------------------------------------------------------

  Void xMotionEstimation          ( TComDataCU*  pcCU,
                                    TComYuv*     pcYuvOrg,
                                    Int          iPartIdx,
                                    RefPicList   eRefPicList,
                                    TComMv*      pcMvPred,
                                    Int          iRefIdxPred,
                                    TComMv&      rcMv,
                                    UInt&        ruiBits,
                                    Distortion&  ruiCost,
                                    Bool         bBi = false  );

  Void xTZSearch                  ( TComDataCU*  pcCU,
                                    TComPattern* pcPatternKey,
                                    Pel*         piRefY,
                                    Int          iRefStride,
                                    TComMv*      pcMvSrchRngLT,
                                    TComMv*      pcMvSrchRngRB,
                                    TComMv&      rcMv,
                                    Distortion&  ruiSAD,
                                    const TComMv *pIntegerMv2Nx2NPred
                                    );

  Void xTZSearchSelective         ( TComDataCU*  pcCU,
                                    TComPattern* pcPatternKey,
                                    Pel*         piRefY,
                                    Int          iRefStride,
                                    TComMv*      pcMvSrchRngLT,
                                    TComMv*      pcMvSrchRngRB,
                                    TComMv&      rcMv,
                                    Distortion&  ruiSAD,
                                    const TComMv *pIntegerMv2Nx2NPred
                                    );

  Void xSetSearchRange            ( TComDataCU*  pcCU,
                                    TComMv&      cMvPred,
                                    Int          iSrchRng,
                                    TComMv&      rcMvSrchRngLT,
                                    TComMv&      rcMvSrchRngRB );

  Void xPatternSearchFast         ( TComDataCU*  pcCU,
                                    TComPattern* pcPatternKey,
                                    Pel*         piRefY,
                                    Int          iRefStride,
                                    TComMv*      pcMvSrchRngLT,
                                    TComMv*      pcMvSrchRngRB,
                                    TComMv&      rcMv,
                                    Distortion&  ruiSAD,
                                    const TComMv* pIntegerMv2Nx2NPred
                                  );

  Void xPatternSearch             ( TComPattern* pcPatternKey,
                                    Pel*         piRefY,
                                    Int          iRefStride,
                                    TComMv*      pcMvSrchRngLT,
                                    TComMv*      pcMvSrchRngRB,
                                    TComMv&      rcMv,
                                    Distortion&  ruiSAD );

  Void xPatternSearchFracDIF      (
                                    Bool         bIsLosslessCoded,
                                    TComDataCU*  pcCU,
                                    TComPattern* pcPatternKey,
                                    Pel*         piRefY,
                                    Int          iRefStride,
                                    TComMv*      pcMvInt,
                                    TComMv&      rcMvHalf,
                                    TComMv&      rcMvQter,
                                    Distortion&  ruiCost
                                   );

  Void xExtDIFUpSamplingH( TComPattern* pcPattern );
  Void xExtDIFUpSamplingQ( TComPattern* pcPatternKey, TComMv halfPelRef );

  // -------------------------------------------------------------------------------------------------------------------
  // T & Q & Q-1 & T-1
  // -------------------------------------------------------------------------------------------------------------------


  Void xEncodeInterResidualQT( const ComponentID compID, TComTU &rTu );
#if SCM_U0106_ACT_TU_SIG
  Void xEstimateInterResidualQTTUCSC( TComYuv* pcResi, Double &rdCost, UInt &ruiBits, Distortion &ruiDist, TComTU &rTu, TComYuv* pcOrgResi, ACTRDTestTypes eACTRDtype DEBUG_STRING_FN_DECLARE(sDebug) );
#endif
  Void xEstimateInterResidualQT( TComYuv* pcResi, Double &rdCost, UInt &ruiBits, Distortion &ruiDist, Distortion *puiZeroDist, TComTU &rTu DEBUG_STRING_FN_DECLARE(sDebug), TComYuv* pcOrgResi = NULL );
  Void xSetInterResidualQTData( TComYuv* pcResi, Bool bSpatial, TComTU &rTu  );

  UInt  xModeBitsIntra ( TComDataCU* pcCU, UInt uiMode, UInt uiPartOffset, UInt uiDepth, const ChannelType compID );
  UInt  xUpdateCandList( UInt uiMode, Double uiCost, UInt uiFastCandNum, UInt * CandModeList, Double * CandCostList );

  // -------------------------------------------------------------------------------------------------------------------
  // compute symbol bits
  // -------------------------------------------------------------------------------------------------------------------

  Void xAddSymbolBitsInter       ( TComDataCU*   pcCU,
                                   UInt&         ruiBits);

  Void  setWpScalingDistParam( TComDataCU* pcCU, Int iRefIdx, RefPicList eRefPicListCur );
  inline  Void  setDistParamComp( ComponentID compIdx )  { m_cDistParam.compIdx = compIdx; }

  Void   xDeriveRun (TComDataCU* pcCU, Pel* pOrg [3],  Pel *pPalette [3],  Pel* pValue, UChar* pSPoint, Pel *pRecoValue[], Pel *pPixelRec[], TCoeff* pRun, UInt uiWidth, UInt uiHeight,  UInt uiStrideOrg, UInt uiPLTSize);
#if SCM_U0096_PLT_ENCODER_IMPROVEMENT
  Double xGetRunBits(TComDataCU* pcCU, Pel *pValue, UInt uiStartPos, UInt uiRun, PLTRunMode cPltRunMode, UInt64 *allBits, UInt64 *indexBits, UInt64 *runBits);
  UInt preCalcRD(TComDataCU* pcCU, Pel *Palette[3], Pel* pSrc[3], UInt uiWidth, UInt uiHeight, UInt uiPLTSize, TComRdCost *pcCost, UInt uiIterNumber);
  Void preCalcRDMerge(TComDataCU* pcCU, Pel *Palette[3], Pel* pSrc[3], UInt uiWidth, UInt uiHeight, UInt uiPLTSize, TComRdCost *pcCost,
    UInt *errorOrig, UInt *errorNew, UInt calcErrBits);
  UInt calcPltIndexPredAndBits(Int iMaxSymbol, UInt uiIdxStart, UInt uiWidth, UInt *predIndex, UInt *currIndex);
  UInt calcPLTEndPosition(UInt copyPixels[], UInt positionInit, UInt run);
  UInt calcPLTStartCopy(UInt positionInit, UInt positionCurrSegment, UInt uiWidth);
  UInt findPltSegment(pltInfoStruct *pltElement, TComDataCU* pcCU, UInt uiIdxStart, UInt uiIndexMaxSize, UInt uiWidth, UInt uiTotal,
    UInt copyPixels[], Int restrictLevelRun, UInt calcErrBits);
  UInt calcPltErrorCopy(UInt uiIdxStart, UInt run, UInt uiWidth, UInt *mode);
  UInt64 calcPltErrorLevel(Int idxStart, UInt run, UInt uiPLTIdx);
  Void modifyPltSegment(UInt uiWidth, UInt uiIdxStart, UInt pltMode, UInt pltIdx, UInt run);
#else
  Double xGetRunBits(TComDataCU* pcCU, Pel *pValue, UInt uiStartPos, UInt uiRun, PLTRunMode cPltRunMode);
#endif
};// END CLASS DEFINITION TEncSearch

//! \}

#endif // __TENCSEARCH__
