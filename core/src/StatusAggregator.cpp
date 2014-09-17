#include "Base.hpp"
#include "StatusAggregator.hpp"


StatusAggregator::StatusAggregator(const QVector<ThresholdT>& thresholdLimits)
  : m_thresholdsLimits(thresholdLimits)
{
  resetData();
}

void StatusAggregator::resetData(void)
{
  m_severityWeightsMap.clear();
  m_statusRatios.clear();
  m_thresholdsLimits.clear();
  m_count = 0;
  m_essentialCount = 0;
  m_nonEssentialTotalWeight = 0;
  m_minSeverity = 0;
  m_maxSeverity = 0;
  m_maxEssential = 0;

  m_severityWeightsMap[ngrt4n::Normal] = 0.0;
  m_severityWeightsMap[ngrt4n::Minor] = 0.0;
  m_severityWeightsMap[ngrt4n::Major] = 0.0;
  m_severityWeightsMap[ngrt4n::Critical] = 0.0;
  m_severityWeightsMap[ngrt4n::Unknown] = 0.0;

  m_statusRatios[ngrt4n::Normal] = 0.0;
  m_statusRatios[ngrt4n::Minor] = 0.0;
  m_statusRatios[ngrt4n::Major] = 0.0;
  m_statusRatios[ngrt4n::Critical] = 0.0;
  m_statusRatios[ngrt4n::Unset] = 0.0;

}

void StatusAggregator::addSeverity(int value, double weight)
{
  if (! Severity(value).isValid())
    value = ngrt4n::Unknown;

  if (weight != 0) {
    m_minSeverity = qMin(m_minSeverity, value);
    m_maxSeverity = qMax(m_maxSeverity, value);
    if (weight == ngrt4n::WEIGHT_MAX) {
      m_essentialCount += 1;
      m_maxEssential = qMax(m_maxEssential, value);
    } else {
      m_severityWeightsMap[value] += weight;
      m_nonEssentialTotalWeight += weight;
    }
  }
  updateThresholds();
  ++m_count;
}

void StatusAggregator::addThresholdLimit(const ThresholdT& th)
{
  m_thresholdsLimits.push_back(th);
  qSort(m_thresholdsLimits.begin(), m_thresholdsLimits.end(), ThresholdLessthanFnt());
}


void StatusAggregator::updateThresholds(void)
{
  if (m_nonEssentialTotalWeight > 0)
    Q_FOREACH(int sev, m_severityWeightsMap.keys()) m_statusRatios[sev] = m_severityWeightsMap[sev] / m_nonEssentialTotalWeight;
  else
    Q_FOREACH(int sev, m_severityWeightsMap.keys()) m_statusRatios[sev] = std::numeric_limits<double>::max();
}

QString StatusAggregator::toString(void)
{
  return QObject::tr(
        "Unknown: %1\%; Critical: %2\%; Major: %3\%; Minor: %4\%; Normal: %5\%; ")
      .arg(QString::number(100 * m_statusRatios[ngrt4n::Unknown]))
      .arg(QString::number(100 * m_statusRatios[ngrt4n::Critical]))
      .arg(QString::number(100 * m_statusRatios[ngrt4n::Major]))
      .arg(QString::number(100 * m_statusRatios[ngrt4n::Minor]))
      .arg(QString::number(100 * m_statusRatios[ngrt4n::Normal]));
}


int StatusAggregator::aggregate(int crule)
{
  m_thresholdExceededMsg.clear();
  if (m_statusRatios.isEmpty())
    return ngrt4n::Normal;

  int result = ngrt4n::Unknown;
  switch (crule) {
  case CalcRules::Average:
    result = weightedAverage();
    break;
  case CalcRules::Weighted:
    result = weightedAverageWithThresholds();
    break;
  case CalcRules::Worst:
  default:
    result = m_maxSeverity;
    break;
  }
  return result;
}


int StatusAggregator::propagate(int sev, int prule)
{
  qint8 result = static_cast<ngrt4n::SeverityT>(sev);
  Severity sevHelper(static_cast<ngrt4n::SeverityT>(sev));
  switch(prule) {
  case PropRules::Increased:
    result = (++sevHelper).value();
    break;
  case PropRules::Decreased:
    result = (--sevHelper).value();
    break;
  default:
    break;
  }
  return result;
}

int StatusAggregator::weightedAverage(void)
{
  double severityScore = 0;
  double weightSum = 0;
  Q_FOREACH(int sev, m_severityWeightsMap.keys()) {
    double weight = m_severityWeightsMap[sev];
    if (weight > 0) {
      severityScore += weight * static_cast<double>(sev);
      weightSum += weight * ngrt4n::WEIGHT_UNIT;
    }
  }
  return qMax(qRound(severityScore / weightSum), m_maxEssential);
}

int StatusAggregator::weightedAverageWithThresholds(void)
{
  int thresholdReached = -1;
  int index = m_thresholdsLimits.size() - 1;

  while (index >= 0 && thresholdReached == -1) {
    ThresholdT th = m_thresholdsLimits[index];
    QMap<int, double>::iterator thvalue = m_statusRatios.find(th.sev_in);
    if (thvalue != m_statusRatios.end() && (*thvalue >= th.weight)) {
      thresholdReached = m_thresholdsLimits[index].sev_out;
      m_thresholdExceededMsg = QObject::tr("%1 events exceeded %2\% and set to %3").arg(Severity(th.sev_in).toString(),
                                                                                        QString::number(100 * th.weight),
                                                                                        Severity(th.sev_out).toString()
                                                                                        );
    }
    --index;
  }

  if (thresholdReached != -1)
    return qMax(thresholdReached, weightedAverage());

  return weightedAverage();
}
