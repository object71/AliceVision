// This file is part of the AliceVision project.
// Copyright (c) 2020 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "sfmStatistics.hpp"

#include <aliceVision/sfm/pipeline/localization/SfMLocalizer.hpp>

#include <aliceVision/sfm/pipeline/regionsIO.hpp>
#include <aliceVision/sfm/pipeline/pairwiseMatchesIO.hpp>
#include <aliceVision/track/TracksBuilder.hpp>


namespace aliceVision {
namespace sfm {

void computeResidualsHistogram(const sfmData::SfMData& sfmData, BoxStats<double>& out_stats, Histogram<double>* out_histogram, const std::set<IndexT>& specificViews)
{
  {
    // Init output params
    out_stats = BoxStats<double>();
    if (out_histogram)
    {
      *out_histogram = Histogram<double>();
    }
  }
  if (sfmData.getLandmarks().empty())
    return;

  // Collect residuals for each observation
  std::vector<double> vec_residuals;
  vec_residuals.reserve(sfmData.structure.size());

  for(const auto &track : sfmData.getLandmarks())
  {
    const aliceVision::sfmData::Observations & observations = track.second.observations;
    for(const auto& obs: observations)
    {
      if(!specificViews.empty())
      {
          if(specificViews.count(obs.first) == 0)
              continue;
      }
      const sfmData::View& view = sfmData.getView(obs.first);
      const aliceVision::geometry::Pose3 pose = sfmData.getPose(view).getTransform();
      const std::shared_ptr<aliceVision::camera::IntrinsicBase> intrinsic = sfmData.getIntrinsics().find(view.getIntrinsicId())->second;
      const Vec2 residual = intrinsic->residual(pose, track.second.X, obs.second.x);
      vec_residuals.push_back(residual.norm());
      //ALICEVISION_LOG_INFO("[AliceVision] sfmtstatistics::computeResidualsHistogram track: " << track.first << ", residual: " << residual.norm());
    }
  }

 // ALICEVISION_LOG_INFO("[AliceVision] sfmtstatistics::computeResidualsHistogram vec_residuals.size(): " << vec_residuals.size());

  if(vec_residuals.empty())
      return;

  out_stats = BoxStats<double>(vec_residuals.begin(), vec_residuals.end());

  if (out_histogram)
  {
    *out_histogram = Histogram<double>(0.0, std::ceil(out_stats.max), std::ceil(out_stats.max)*2);
    out_histogram->Add(vec_residuals.begin(), vec_residuals.end());
  }
}


void computeObservationsLengthsHistogram(const sfmData::SfMData& sfmData, BoxStats<double>& out_stats, int& overallNbObservations, Histogram<double>* out_histogram, const std::set<IndexT>& specificViews)
{
  {
    // Init output params
    out_stats = BoxStats<double>();
    if (out_histogram)
    {
      *out_histogram = Histogram<double>();
    }
  }
  if (sfmData.getLandmarks().empty())
    return;

  // Collect tracks size: number of 2D observations per 3D points
  std::vector<int> nbObservations;
  nbObservations.reserve(sfmData.getLandmarks().size());

  for(const auto& landmark : sfmData.getLandmarks())
  {
    const aliceVision::sfmData::Observations & observations = landmark.second.observations;
    if (!specificViews.empty())
    {
        int nbObsSpecificViews = 0;
        for(const auto& obs: observations)
        {
            if(specificViews.count(obs.first) == 1)
            {
                ++nbObsSpecificViews;
            }
        }
        if(nbObsSpecificViews > 0)
        {
            nbObservations.push_back(observations.size());
        }
    }
    else
    {
        nbObservations.push_back(observations.size());
    }
    overallNbObservations += observations.size();
  }

  if(nbObservations.empty())
      return;

  out_stats = BoxStats<double>(nbObservations.begin(), nbObservations.end());

  if (out_histogram)
  {
    *out_histogram = Histogram<double>(out_stats.min, out_stats.max + 1, out_stats.max - out_stats.min + 1);
    out_histogram->Add(nbObservations.begin(), nbObservations.end());
  }
}

void computeLandmarksPerViewHistogram(const sfmData::SfMData& sfmData, BoxStats<double>& out_stats, Histogram<double>* out_histogram)
{
    {
        // Init output params
        out_stats = BoxStats<double>();
        if (out_histogram)
        {
            *out_histogram = Histogram<double>();
        }
    }
    if(sfmData.getLandmarks().empty())
        return;

    std::map<IndexT, int> nbLandmarksPerView;

    for(const auto& landmark: sfmData.getLandmarks())
    {
        for(const auto& obsIt: landmark.second.observations)
        {
            const auto& viewId = obsIt.first;
            auto it = nbLandmarksPerView.find(viewId);
            if(it != nbLandmarksPerView.end())
                ++(it->second);
            else
                it->second = 1;
        }
    }
    if(nbLandmarksPerView.empty())
        return;

    // Collect tracks size: number of 2D observations per 3D points
    std::vector<int> nbLandmarksPerViewVec;
    for(const auto& it: nbLandmarksPerView)
        nbLandmarksPerViewVec.push_back(it.second);

    out_stats = BoxStats<double>(nbLandmarksPerViewVec.begin(), nbLandmarksPerViewVec.end());

    if (out_histogram)
    {
        //*out_histogram = Histogram<double>(0, sfmData.getViews().size(), sfmData.getViews().size());
        *out_histogram = Histogram<double>(out_stats.min, (out_stats.max + 1), 10);
        out_histogram->Add(nbLandmarksPerViewVec.begin(), nbLandmarksPerViewVec.end());
    }
}


void computeLandmarksPerView(const sfmData::SfMData& sfmData, std::vector<int>& out_nbLandmarksPerView)
{
    {
        out_nbLandmarksPerView.clear();
    }
    if(sfmData.getLandmarks().empty())
        return;

    std::map<IndexT, int> nbLandmarksPerView;

    for(const auto& landmark: sfmData.getLandmarks())
    {
        for(const auto& obsIt: landmark.second.observations)
        {
            const auto& viewId = obsIt.first;
            auto it = nbLandmarksPerView.find(viewId);
            if(it != nbLandmarksPerView.end())
                ++(it->second);
            else
                nbLandmarksPerView[viewId] = 1;
        }
    }
    if(nbLandmarksPerView.empty())
        return;

    for(const auto& it: nbLandmarksPerView)
        out_nbLandmarksPerView.push_back(it.second);
}

void computeFeatMatchPerView(const sfmData::SfMData& sfmData, std::vector<std::size_t>& out_featPerView, std::vector<std::size_t>& out_matchPerView)
{
    const auto descTypesTmp = sfmData.getLandmarkDescTypes();
    const std::vector<feature::EImageDescriberType> describerTypes(descTypesTmp.begin(), descTypesTmp.end());

    // features reading
    feature::FeaturesPerView featuresPerView;
    if(!sfm::loadFeaturesPerView(featuresPerView, sfmData, sfmData.getFeaturesFolders(), describerTypes))
    {
        ALICEVISION_LOG_ERROR("Invalid features.");
        return;
    }

    // matches reading
    matching::PairwiseMatches pairwiseMatches;
    if(!sfm::loadPairwiseMatches(pairwiseMatches, sfmData, sfmData.getMatchesFolders(), describerTypes, 0, 0, true))
    {
        ALICEVISION_LOG_ERROR("Unable to load matches.");
        return;
    }

    track::TracksMap tracks;
    track::TracksPerView tracksPerView;

    track::TracksBuilder tracksBuilder;
    tracksBuilder.build(pairwiseMatches);
    tracksBuilder.exportToSTL(tracks);

    // Init tracksPerView to have an entry in the map for each view (even if there is no track at all)
    for(const auto& viewIt: sfmData.views)
    {
        // create an entry in the map
        tracksPerView[viewIt.first];
    }
    track::computeTracksPerView(tracks, tracksPerView);

    out_featPerView.reserve(sfmData.getViews().size());
    out_matchPerView.reserve(sfmData.getViews().size());
    for(const auto& viewIt: sfmData.getViews())
    {
        const auto viewId = viewIt.first;

        if(featuresPerView.viewExist(viewId))
            out_featPerView.push_back(featuresPerView.getNbFeatures(viewId));
        else
            out_featPerView.push_back(0);

        if(tracksPerView.count(viewId))
            out_matchPerView.push_back(tracksPerView.at(viewId).size());
        else
            out_matchPerView.push_back(0);
    }
}

void computeScaleHistogram(const sfmData::SfMData& sfmData, BoxStats<double>& out_stats, Histogram<double>* out_histogram, const std::set<IndexT>& specificViews)
{
    {
      // Init output params
      out_stats = BoxStats<double>();
      if (out_histogram)
      {
        *out_histogram = Histogram<double>();
      }
    }
    if(sfmData.getLandmarks().empty())
      return;

    // Collect tracks size: number of 2D observations per 3D points
    std::vector<double> vec_scaleObservations;
    vec_scaleObservations.reserve(sfmData.getLandmarks().size());
    for(const auto& landmark: sfmData.getLandmarks())
    {
        const aliceVision::sfmData::Observations & observations = landmark.second.observations;

        for(const auto& obs: observations)
        {
            if(!specificViews.empty())
            {
                if(specificViews.count(obs.first) == 0)
                    continue;
            }
            vec_scaleObservations.push_back(obs.second.scale);
            // ALICEVISION_LOG_INFO("[AliceVision] sfmtstatistics::computescaleHistogram landmark: " << landmark.first << ", scale: " << obs.second.scale);
        }
    }

    if(vec_scaleObservations.empty())
        return;

   // ALICEVISION_LOG_INFO("[AliceVision] sfmtstatistics::computescaleHistogram vec_scaleObservations.size(): " << vec_scaleObservations.size());

    out_stats = BoxStats<double>(vec_scaleObservations.begin(), vec_scaleObservations.end());
   // ALICEVISION_LOG_INFO("[AliceVision] sfmtstatistics::computescaleHistogram out_stats " << out_stats);

    if (out_histogram)
    {
      size_t maxValue = std::ceil(out_stats.max);
      *out_histogram = Histogram<double>(0.0, double(maxValue), maxValue +1);
      out_histogram->Add(vec_scaleObservations.begin(), vec_scaleObservations.end());
    }
}

void computeResidualsPerView(const sfmData::SfMData& sfmData, int& nbViews, std::vector<double>& nbResidualsPerViewMin,
                              std::vector<double>& nbResidualsPerViewMax, std::vector<double>& nbResidualsPerViewMean,
                              std::vector<double>& nbResidualsPerViewMedian, std::vector<double>& nbResidualsPerViewFirstQuartile,
                              std::vector<double>& nbResidualsPerViewThirdQuartile)
{
    if(sfmData.getLandmarks().empty())
      return;

    for(const auto &viewIt : sfmData.getViews())
    {
        BoxStats<double> residualStats;
        Histogram<double> residual_histogram = Histogram<double>();
        computeResidualsHistogram(sfmData, residualStats, &residual_histogram, {viewIt.first});
        nbResidualsPerViewMin.push_back(residualStats.min);
        nbResidualsPerViewMax.push_back(residualStats.max);
        nbResidualsPerViewMean.push_back(residualStats.mean);
        nbResidualsPerViewMedian.push_back(residualStats.median);
        nbResidualsPerViewFirstQuartile.push_back(residualStats.firstQuartile);
        nbResidualsPerViewThirdQuartile.push_back(residualStats.thirdQuartile);
        nbViews++;
    }

    // assert(!nbResidualsPerViewMin.empty() && !nbResidualsPerViewMax.empty() && !nbResidualsPerViewMean.empty() && !nbResidualsPerViewMedian.empty());
}

void computeObservationsLengthsPerView(const sfmData::SfMData& sfmData, int& nbViews, std::vector<double>& nbObservationsLengthsPerViewMin,
                                      std::vector<double>& nbObservationsLengthsPerViewMax, std::vector<double>& nbObservationsLengthsPerViewMean,
                                      std::vector<double>& nbObservationsLengthsPerViewMedian, std::vector<double>& nbObservationsLengthsPerViewFirstQuartile,
                                       std::vector<double>& nbObservationsLengthsPerViewThirdQuartile)
{
    if(sfmData.getLandmarks().empty())
      return;

    int overallNbObservations = 0;

    for(const auto &viewIt : sfmData.getViews())
    {
        BoxStats<double> observationsLengthsStats;
        Histogram<double> observationsLengths_histogram = Histogram<double>();
        computeObservationsLengthsHistogram(sfmData, observationsLengthsStats, overallNbObservations, &observationsLengths_histogram, {viewIt.first});
        nbObservationsLengthsPerViewMin.push_back(observationsLengthsStats.min);
        nbObservationsLengthsPerViewMax.push_back(observationsLengthsStats.max);
        nbObservationsLengthsPerViewMean.push_back(observationsLengthsStats.mean);
        nbObservationsLengthsPerViewMedian.push_back(observationsLengthsStats.median);
        nbObservationsLengthsPerViewFirstQuartile.push_back(observationsLengthsStats.firstQuartile);
        nbObservationsLengthsPerViewThirdQuartile.push_back(observationsLengthsStats.thirdQuartile);

        nbViews++;
    }

}

}
}
