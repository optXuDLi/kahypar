/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
#include "lib/core/PolicyRegistry.h"
#include "lib/macros.h"
#include "partition/Configuration.h"

using core::PolicyBase;

namespace partition {
struct StoppingPolicy : PolicyBase {
 protected:
  StoppingPolicy() { }
};

class NumberOfFruitlessMovesStopsSearch : public StoppingPolicy {
 public:
  bool searchShouldStop(const int, const Configuration& config,
                        const double, const HyperedgeWeight, const HyperedgeWeight) noexcept {
    return _num_moves >= config.fm_local_search.max_number_of_fruitless_moves;
  }

  void resetStatistics() noexcept {
    _num_moves = 0;
  }

  template <typename Gain>
  void updateStatistics(const Gain) noexcept {
    ++_num_moves;
  }

 private:
  int _num_moves = 0;
};

class RandomWalkModelStopsSearch : public StoppingPolicy {
 public:
  bool searchShouldStop(const int, const Configuration& config, const double beta,
                        const HyperedgeWeight, const HyperedgeWeight) noexcept {
    DBG(false, "step=" << _num_steps);
    DBG(false, _num_steps << "*" << _expected_gain << "^2=" << _num_steps * _expected_gain * _expected_gain);
    DBG(false, config.fm_local_search.alpha << "*" << _expected_variance << "+" << beta << "="
        << config.fm_local_search.alpha * _expected_variance + beta);
    DBG(false, "return=" << ((_num_steps * _expected_gain * _expected_gain >
                              config.fm_local_search.alpha * _expected_variance + beta) && (_num_steps != 1)));
    return (_num_steps * _expected_gain * _expected_gain >
            config.fm_local_search.alpha * _expected_variance + beta) && (_num_steps != 1);
  }

  void resetStatistics() noexcept {
    _num_steps = 0;
    _expected_gain = 0.0;
    _expected_variance = 0.0;
    _sum_gains = 0.0;
  }

  template <typename Gain>
  void updateStatistics(const Gain gain) noexcept {
    ++_num_steps;
    _sum_gains += gain;
    _expected_gain = _sum_gains / _num_steps;
    // http://de.wikipedia.org/wiki/Standardabweichung#Berechnung_f.C3.BCr_auflaufende_Messwerte
    if (_num_steps > 1) {
      _MkMinus1 = _Mk;
      _Mk = _MkMinus1 + (gain - _MkMinus1) / _num_steps;
      _SkMinus1 = _Sk;
      _Sk = _SkMinus1 + (gain - _MkMinus1) * (gain - _Mk);
      _expected_variance = _Sk / (_num_steps - 1);
    } else {
      // everything else is already reset in resetStatistics
      _Mk = static_cast<double>(gain);
      _Sk = 0.0;
    }
  }

 private:
  int _num_steps = 0;
  double _expected_gain = 0.0;
  double _expected_variance = 0.0;
  double _sum_gains = 0.0;
  double _Mk = 0.0;
  double _MkMinus1 = 0.0;
  double _Sk = 0.0;
  double _SkMinus1 = 0.0;
};


class nGPRandomWalkStopsSearch : public StoppingPolicy {
 public:
  bool searchShouldStop(const int num_moves_since_last_improvement,
                        const Configuration& config, const double beta,
                        const HyperedgeWeight best_cut, const HyperedgeWeight cut) noexcept {
    return num_moves_since_last_improvement
           >= config.fm_local_search.alpha * ((_sum_gains_squared * num_moves_since_last_improvement)
                                              / (2.0 * (static_cast<double>(best_cut) - cut)
                                                 * (static_cast<double>(best_cut) - cut) - 0.5)
                                              + beta);
  }

  void resetStatistics() noexcept {
    _sum_gains_squared = 0.0;
  }

  template <typename Gain>
  void updateStatistics(const Gain gain) noexcept {
    _sum_gains_squared += gain * gain;
  }

 private:
  double _sum_gains_squared = 0.0;
};
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
