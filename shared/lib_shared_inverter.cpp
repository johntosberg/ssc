#include "lib_shared_inverter.h"
#include "lib_util.h"
#include <algorithm>

SharedInverter::SharedInverter(int inverterType, size_t numberOfInverters,
	sandia_inverter_t * sandiaInverter, partload_inverter_t * partloadInverter, ond_inverter * ondInverter)
{
	m_inverterType = inverterType;
	m_numInverters = numberOfInverters;
	m_sandiaInverter = sandiaInverter;
	m_partloadInverter = partloadInverter;
	m_ondInverter = ondInverter;
	m_tempEnabled = false;
}

bool sortByVoltage(std::vector<double> i, std::vector<double> j)
{
	return (i[0] < j[0]);
}

int SharedInverter::setTempDerateCurves(std::vector<std::vector<double>> derateCurves)
{
	m_thermalDerateCurves.clear();

	// Check derate curves have V > 0, and that for each pair T > -273, slope < 0
	for (size_t r = 0; r < derateCurves.size(); r++) {
		if (derateCurves[r][0] <= 0.) return (int)r + 1;
		size_t tempSlopeEntries = derateCurves[r].size() - 1;
		if ((tempSlopeEntries % 2) != 0) return (int)r + 1;
		for (size_t p = 0; p < tempSlopeEntries / 2; p++) {
			if ( derateCurves[r][2*p+1] <= -273. || derateCurves[r][2*p+2] >= 0.)  return (int)r + 1;
		}
		m_thermalDerateCurves.push_back(derateCurves[r]);
	}

	// Sort by DC voltage
	std::sort(m_thermalDerateCurves.begin(), m_thermalDerateCurves.end(), sortByVoltage);
	
	if (m_thermalDerateCurves.size() > 0 ) m_tempEnabled = true;
	return 0;
}

std::vector<std::vector<double>> SharedInverter::getTempDerateCurves() {
	return m_thermalDerateCurves;
}

void SharedInverter::findPointOnCurve(size_t idx, double T, double& startT, double& slope) {
	size_t p = 0;
	while (2 * p + 2 < m_thermalDerateCurves[idx].size() && T >= m_thermalDerateCurves[idx][2 * p + 1]) {
		p++;
	}
	if (2 * p + 2 >= m_thermalDerateCurves[idx].size()) {
		p--;
	}
	startT = m_thermalDerateCurves[idx][2 * p + 1];
	slope = m_thermalDerateCurves[idx][2 * p + 2];
}

void SharedInverter::calculateTempDerate(double V, double T, double& pAC, double& eff, double& loss)
{
	if (eff == 0. || pAC == 0.) return;

	double slope = 0.0;
	double startT = 0.0;
	double Vdc = 0.0;
	double slope2 = 0.0;
	double startT2 = 0.0;
	double Vdc2 = 0.0;

	// Find the appropriate derate curve depending on DC voltage
	size_t idx = 0;
	double deltaT = 0.0;
	double slopeInterpolated = 0.0;
	double startTInterpolated = 0.0;

	while (idx < m_thermalDerateCurves.size() && V > m_thermalDerateCurves[idx][0]) {
		idx++;
	}
	if (m_thermalDerateCurves.size() == 1) {
		Vdc2 = m_thermalDerateCurves[0][0];
		startTInterpolated = m_thermalDerateCurves[0][1];
		slopeInterpolated = m_thermalDerateCurves[0][2];
	}
	// Use temp and slope of lower and upper curves for interpolation if they both exist
	else if (idx > 0 && idx < m_thermalDerateCurves.size()) {
		Vdc2 = m_thermalDerateCurves[idx][0];
		Vdc = m_thermalDerateCurves[idx - 1][0];
		double startTGuess = 0.0;
		double slopeGuess = 0.0;
		size_t n = std::max(m_thermalDerateCurves[idx].size() / 2, m_thermalDerateCurves[idx - 1].size() / 2);
		size_t count = 0;
		while (T > startTGuess && count < n ) {
			findPointOnCurve(idx, startT2, startT2, slope2);
			findPointOnCurve(idx-1, startT, startT, slope);
			startTGuess = (startT2 - startT) / (Vdc2 - Vdc)*(V - Vdc2) + startT2;
			slopeGuess = (slope2 - slope) / (Vdc2 - Vdc)*(V - Vdc2) + slope2;
			if (T > startTGuess) {
				startTInterpolated = startTGuess;
				slopeInterpolated = slopeGuess;
				count++;
			}
		}
	}
	// otherwise extrapolate using first start temps of each curve in order to avoid inconsistent start temps
	else {
		if (idx == 0) {
			Vdc2 = m_thermalDerateCurves[idx][0];
			findPointOnCurve(idx, -273, startT2, slope2);
			Vdc = m_thermalDerateCurves[idx + 1][0];
			findPointOnCurve(idx + 1, -273, startT, slope);
			startTInterpolated = (startT2 - startT) / (Vdc2 - Vdc)*(V - Vdc2) + startT2;
			slopeInterpolated = (slope2 - slope) / (Vdc2 - Vdc)*(V - Vdc2) + slope2;
		}
		else {
			Vdc2 = m_thermalDerateCurves[idx-1][0];
			findPointOnCurve(idx - 1, -273, startT2, slope2);
			Vdc = m_thermalDerateCurves[idx - 2][0];
			findPointOnCurve(idx - 2, -273, startT, slope);
			startTInterpolated = (startT2 - startT) / (Vdc2 - Vdc)*(V - Vdc2) + startT2;
			slopeInterpolated = (slope2 - slope) / (Vdc2 - Vdc)*(V - Vdc2) + slope2;
		}
	}
	deltaT = T - startTInterpolated;

	// If less than start temp, no derating
	if (deltaT <= 0) return;

	// If slope is positive, set to zero with no derating
	if (slopeInterpolated >= 0) return;
	if (slopeInterpolated < -1) slopeInterpolated = -1;

	// Power in units of W, eff as ratio
	double pDC = pAC/eff;
	eff += deltaT* slopeInterpolated;
	if (eff < 0) eff = 0.;
	loss = pAC - (pDC * eff);
	pAC = pDC * eff;	
}

void SharedInverter::calculateACPower(const double powerDC_Watts, const double DCStringVoltage, double T)
{
	double P_par, P_lr;
	bool negativePower = powerDC_Watts < 0 ? true : false;


	dcWiringLoss_ond_kW = 0.0;
	acWiringLoss_ond_kW = 0.0;

	// Power quantities go in and come out in units of W
	if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
		m_sandiaInverter->acpower(std::fabs(powerDC_Watts) / m_numInverters, DCStringVoltage, &powerAC_kW, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW);
	else if (m_inverterType == PARTLOAD_INVERTER)
		m_partloadInverter->acpower(std::fabs(powerDC_Watts) / m_numInverters, &powerAC_kW, &P_lr, &P_par, &efficiencyAC, &powerClipLoss_kW, &powerNightLoss_kW);
	else if (m_inverterType == OND_INVERTER)
		m_ondInverter->acpower(std::fabs(powerDC_Watts) / m_numInverters,DCStringVoltage, T, &powerAC_kW, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW, &dcWiringLoss_ond_kW, &acWiringLoss_ond_kW);


	double tempLoss = 0.0;
	if (m_tempEnabled){
		calculateTempDerate(DCStringVoltage, T, powerAC_kW, efficiencyAC, tempLoss);
	}

	// Convert units to kW
	powerDC_kW = powerDC_Watts * util::watt_to_kilowatt;
	powerAC_kW *= m_numInverters * util::watt_to_kilowatt;
	powerClipLoss_kW *= m_numInverters * util::watt_to_kilowatt;
	powerConsumptionLoss_kW *= m_numInverters * util::watt_to_kilowatt;
	powerNightLoss_kW *= m_numInverters * util::watt_to_kilowatt;
	powerTempLoss_kW = tempLoss * m_numInverters * util::watt_to_kilowatt;
	powerLossTotal_kW = powerDC_kW - powerAC_kW;
	efficiencyAC *= 100;
	dcWiringLoss_ond_kW *= m_numInverters * util::watt_to_kilowatt;
	acWiringLoss_ond_kW *= m_numInverters * util::watt_to_kilowatt;

	// In event shared inverter is charging a battery only, need to re-convert to negative power
	if (negativePower) {
		powerAC_kW *= -1.0;
	}
}

double SharedInverter::getInverterDCNominalVoltage()
{
	if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
		return m_sandiaInverter->Vdco;
	else if (m_inverterType == PARTLOAD_INVERTER)
		return m_partloadInverter->Vdco;
	else if (m_inverterType == OND_INVERTER)
		return m_ondInverter->VNomEff[1];
	else
		return 0.;
}

double SharedInverter::getMaxPowerEfficiency()
{
	if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
		calculateACPower(m_sandiaInverter->Paco, m_sandiaInverter->Vdco, 0.0);
	else if (m_inverterType == PARTLOAD_INVERTER)
		calculateACPower(m_partloadInverter->Paco, m_partloadInverter->Vdco, 0.0);
	else if (m_inverterType == OND_INVERTER)
		calculateACPower(m_ondInverter->PMaxOUT, m_ondInverter->VAbsMax, 0.0);

	return efficiencyAC;
}