#include <iterator>
#include <cmath>

#include "lib_fuel_cell.h" 

FuelCell::FuelCell() { /* Nothing to do */ };

FuelCell::FuelCell(double unitPowerMax_kW, double unitPowerMin_kW, double startup_hours, double shutdown_hours,
	double dynamicResponseUp_kWperHour, double dynamicResponseDown_kWperHour,
	double degradation_kWperHour, double degradationRestart_kW,
	size_t replacementOption, double replacement_percent, std::vector<size_t> replacementSchedule,
	util::matrix_t<size_t> shutdownTable,
	util::matrix_t<double> efficiencyTable,
	double lowerHeatingValue_BtuPerFt3, double higherHeatingValue_BtuPerFt3, double availableFuel_Mcf,
	int shutdownOption, double dt_hour) :
	dt_hour(dt_hour),
	m_unitPowerMax_kW(unitPowerMax_kW), 
	m_unitPowerMin_kW(unitPowerMin_kW), 
	m_startup_hours(startup_hours), 
	m_shutdown_hours(shutdown_hours),
	m_dynamicResponseUp_kWperHour(dynamicResponseUp_kWperHour), 
	m_dynamicResponseDown_kWperHour(dynamicResponseDown_kWperHour),
	m_degradation_kWperHour(degradation_kWperHour), 
	m_degradationRestart_kW(degradationRestart_kW),
	m_scheduledShutdowns(shutdownTable),
	m_replacementOption(replacementOption), 
	m_replacement_percent(replacement_percent * 0.01), 
	m_replacementSchedule(replacementSchedule),
	m_efficiencyTable(efficiencyTable), 
	m_lowerHeatingValue_BtuPerFt3(lowerHeatingValue_BtuPerFt3),
	m_higherHeatingValue_BtuPerFt3(higherHeatingValue_BtuPerFt3),
	m_availableFuel_MCf(availableFuel_Mcf), 
	m_shutdownOption(shutdownOption), 
	m_powerMax_kW(unitPowerMax_kW), 
	m_power_kW(0), 
	m_powerPrevious_kW(0), 
	m_replacementCount(0)
{
	// Calculate fuel consumption based on inputs
	for (size_t r = 0; r < m_efficiencyTable.nrows(); r++) {

		// Convert to 0-1 from 0 - 100
		m_efficiencyTable.at(r, 0) *= 0.01;
		m_efficiencyTable.at(r, 1) *= 0.01;
		m_efficiencyTable.at(r, 2) *= 0.01;
	
		double fuelConsumption_Mcf = 0.0;

		if (m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PERCENT_MAX) > 0 && 
			m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PRECENT_ELECTRICAL_EFFICIENCY) > 0) {
			// Fuel consumption Btu = dt_hour * operating power kW / (efficiency at LHV * Fuel HHV / Fuel LHV) * BTU_PER_KWH
			double fuelConsumption_Btu = BTU_PER_KWH * dt_hour * m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PERCENT_MAX) * m_unitPowerMax_kW / (m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PRECENT_ELECTRICAL_EFFICIENCY) * m_higherHeatingValue_BtuPerFt3 / m_lowerHeatingValue_BtuPerFt3);
			fuelConsumption_Mcf = BTU_TO_MCF(fuelConsumption_Btu, m_lowerHeatingValue_BtuPerFt3);
		}

		m_fuelConsumptionMap_MCf[m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PERCENT_MAX)] = (fuelConsumption_Mcf);
		m_efficiencyMap[m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PERCENT_MAX)] = m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PRECENT_ELECTRICAL_EFFICIENCY);
		m_heatRecoveryMap[m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PERCENT_MAX)] = m_efficiencyTable.at(r, FC_EFFICIENCY_COLUMN::PERCENT_HEAT_RECOVERY);
	}

	// Assumption: Fuel Cell is not startup up initially
	init();
}

FuelCell::~FuelCell(){ /* Nothing to do */}


FuelCell::FuelCell(const FuelCell &fuelCell) : 
	dt_hour(fuelCell.dt_hour),
	m_unitPowerMax_kW(fuelCell.m_unitPowerMax_kW),
	m_unitPowerMin_kW(fuelCell.m_unitPowerMin_kW),
	m_startup_hours(fuelCell.m_startup_hours),
	m_shutdown_hours(fuelCell.m_shutdown_hours),
	m_dynamicResponseUp_kWperHour(fuelCell.m_dynamicResponseUp_kWperHour),
	m_dynamicResponseDown_kWperHour(fuelCell.m_dynamicResponseDown_kWperHour),
	m_degradation_kWperHour(fuelCell.m_degradation_kWperHour), 
	m_degradationRestart_kW(fuelCell.m_degradationRestart_kW),
	m_scheduledShutdowns(fuelCell.m_scheduledShutdowns),
	m_replacement_percent(fuelCell.m_replacement_percent * 0.01), 
	m_efficiencyTable(fuelCell.m_efficiencyTable), 
	m_lowerHeatingValue_BtuPerFt3(fuelCell.m_lowerHeatingValue_BtuPerFt3),
	m_higherHeatingValue_BtuPerFt3(fuelCell.m_higherHeatingValue_BtuPerFt3),
	m_availableFuel_MCf(fuelCell.m_availableFuel_MCf), 
	m_shutdownOption(fuelCell.m_shutdownOption) 
{
	init();
}

void FuelCell::init() {
	m_startingUp = false;
	m_startedUp = false;
	m_hoursSinceStart = 0;
	m_shutDown = false;
	m_shuttingDown = false;
	m_hoursSinceStop = 0;
	m_hoursRampUp = std::ceilf((float)(m_unitPowerMin_kW / m_dynamicResponseUp_kWperHour));
	m_powerMax_kW = m_unitPowerMax_kW;
	m_powerThermal_kW = 0;
	m_powerPrevious_kW = 0;
	m_fuelConsumed_MCf = 0;
	m_replacementCount = 0;
	m_hour = 0;
	m_year = 0;

	// In event of 0 startup hours, assume fuel cell is running at idle
	if (m_startup_hours == 0) {
		m_power_kW = m_unitPowerMin_kW;
	}

	
}

bool FuelCell::isStarting() {
	return m_startingUp;
}

bool FuelCell::isRunning() {
	return m_startedUp;
}

bool FuelCell::isShutDown() {
	return m_shutDown;
}

bool FuelCell::isShuttingDown() {
	return m_shuttingDown;
}

double FuelCell::interpolateMap(double key, std::map<double, double> mapDouble) {
	
	double p1, p2, f1, f2, f, m;
	p1 = p2 = f1 = f2 = f = m = 0;

	for (auto fc = mapDouble.begin(); fc != mapDouble.end(); fc++) {
		auto fc_next = std::next(fc, 1);
		auto fc_end = mapDouble.rbegin();

		if (key == fc->first) {
			f = fc->second;
			break;
		}
		else if (key == fc_next->first) {
			f = fc_next->second;
			break;
		}
		// interpolate to get the fuel consumption
		else if (key > fc->first && key < fc_next->first) {
			p1 = fc->first;
			p2 = fc_next->first;
			f1 = fc->second;
			f2 = fc_next->second;

			if (fabs(p2 - p1) > 0) {
				m = (f2 - f1) / (p2 - p1);
				f = f1 + m * key;
			}
			break;
		}
		// if percent is higher than the max key, return the max fuel consumption
		else if (key > fc_end->first) {
			f = fc_end->second;
			break;
		}
	}
	return f;
}

void FuelCell::calculateEfficiencyCurve(double percent) {
	if (!isShutDown()) {
		m_fuelConsumed_MCf = interpolateMap(percent, m_fuelConsumptionMap_MCf);
		m_efficiency_percent = interpolateMap(percent, m_efficiencyMap);
		m_heatRecovery_percent = interpolateMap(percent, m_heatRecoveryMap);
	}
}

double FuelCell::getPercentLoad() {
	return m_power_kW / (m_unitPowerMax_kW);
}

void FuelCell::checkPowerResponse() {
	
	// Calculate ramp rate (kW/hr)
	double dP = (m_power_kW - m_powerPrevious_kW) / dt_hour;
	double dP_max = 0.0;

	// Calculate maximum ramp rate up (kW/hr)
	if (dP > 0) {
		dP_max = fmin(fabs(dP), m_dynamicResponseUp_kWperHour);
	}
	// Calculate mmaximum ramp rate down (kW/hr)
	else {
		dP_max = fmin(fabs(dP), m_dynamicResponseDown_kWperHour);
	}
	double sign = fabs(dP) > 0 ? dP / fabs(dP) : 1.0;

	// Limit output to the minimum of the power requested and ramp rate limit
	if (sign > 0) {
		m_power_kW = fmin(m_power_kW, (m_powerPrevious_kW + (dP_max * dt_hour * sign)));
	}
	// Limit output to the maximum of the power requested and ramp rate limit
	else {
		m_power_kW = fmax(m_power_kW, (m_powerPrevious_kW + (dP_max * dt_hour * sign)));
	}
}
double FuelCell::getPower() {
	return m_power_kW;
}
double FuelCell::getPowerThermal() {
	return m_powerThermal_kW;
}
double FuelCell::getMaxPowerOriginal() {
	return m_unitPowerMax_kW;
}
double FuelCell::getMaxPower() {
	return m_powerMax_kW;
}
double FuelCell::getFuelConsumption() {
	return m_fuelConsumed_MCf;
}
double FuelCell::getAvailableFuel() {
	return m_availableFuel_MCf;
}
double FuelCell::getElectricalEfficiency() {
	return m_efficiency_percent;
}
double FuelCell::getHeatRecoveryEfficiency() {
	return m_heatRecovery_percent;
}
int FuelCell::getTotalReplacements() {
	return m_replacementCount;
}
void FuelCell::resetReplacements() {
	m_replacementCount = 0;
}

void FuelCell::setSystemProperties(double nameplate_kW, double min_kW, double startup_hours, double shutdown_hours,
	double dynamicResponseUp_kWperHour, double dynamicResponseDown_kWperHour) {

	m_unitPowerMax_kW = nameplate_kW;
	m_unitPowerMin_kW = min_kW;
	m_startup_hours = startup_hours;
	m_shutdown_hours = shutdown_hours;
	m_dynamicResponseUp_kWperHour = dynamicResponseUp_kWperHour;
	m_dynamicResponseDown_kWperHour = dynamicResponseDown_kWperHour;
	m_powerMax_kW = m_unitPowerMax_kW;

}
void FuelCell::setReplacementOption(size_t replacementOption) {
	m_replacementOption = replacementOption;
}
void FuelCell::setReplacementCapacity(double replacement_percent) {
	m_replacement_percent = replacement_percent * 0.01;
}
void FuelCell::setDegradationkWPerHour(double degradation_kWPerHour) {
	m_degradation_kWperHour = degradation_kWPerHour;
}
void FuelCell::setDegradationRestartkW(double degradation_kW) {
	m_degradationRestart_kW = degradation_kW;
}
void FuelCell::setScheduledShutdowns(util::matrix_t<size_t> shutdowns) {
	m_scheduledShutdowns = shutdowns;
}
void FuelCell::setStartupHours(double startup_hours) {
	m_startup_hours = startup_hours;
	// Assume that this function is only called at the beginning of a simulation and implies that fuel cell already running
	if (startup_hours == 0) {
		m_power_kW = m_unitPowerMin_kW;
	}
}
void FuelCell::setShutdownOption(int shutdownOption) {
	m_shutdownOption = shutdownOption;
}

void FuelCell::checkStatus(double power_kW) {

	// Check if starting up
	if (!isShuttingDown() && !isRunning() &&
		(power_kW > 0 || isStarting()) && m_availableFuel_MCf > 0 && m_powerMax_kW > m_unitPowerMin_kW) {
		m_hoursSinceStart += dt_hour;

		// Fully started once past the startup hour criteria
		if (m_hoursSinceStart > m_startup_hours) {
			m_startedUp = true;
			m_startingUp = false;
			m_power_kW = power_kW;
		}
		else if (m_hoursSinceStart <= m_startup_hours) {
			m_startingUp = true;
			m_shuttingDown = false;
			m_shutDown = false;
			m_hoursSinceStop = 0;
		}
	}  
	// Initialize power
	else if (isRunning()) {
		m_hoursSinceStart += dt_hour;
		m_power_kW = power_kW;
	}

	// Check if is shutting down
	checkMinTurndown();	
	if (isShuttingDown()) {
		m_power_kW = 0;
		m_hoursSinceStop += dt_hour;
	}
	// Scheduled shutdowns
	else if (m_scheduledShutdowns.length() > 0 && !m_shutDown) {

		// Turn off the fuel cell if the hour is one of the scheduled restart hours
		for (size_t r = 0; r < m_scheduledShutdowns.nrows(); r++) {
			double shutdown_hourOfYear = (double)m_scheduledShutdowns.at(r, FuelCell::FC_SHUTDOWN_COLUMN::HOUR);
			double duration_hours = (double)m_scheduledShutdowns.at(r, FuelCell::FC_SHUTDOWN_COLUMN::DURATION);

			if (duration_hours > 0) {

				if (m_hour == shutdown_hourOfYear) {
					m_shuttingDown = true;
					m_startingUp = false;
					m_startedUp = false;
					m_hoursSinceStart = 0;
					m_hoursSinceStop = 0;
				}

				if (m_hour >= shutdown_hourOfYear && m_hour < shutdown_hourOfYear + duration_hours) {
					m_power_kW = 0;
					m_hoursSinceStop += dt_hour;
					break;
				}
			}
		}
	}
	
	if (m_hoursSinceStop > m_shutdown_hours) {
		m_shuttingDown = false;
		m_shutDown = true;
	}
}

// Assume that min turndown trumps dynamic response, i.e, that fuel cell
// can go from 0 to min turndown instantaneously.
void FuelCell::checkMinTurndown() {
	
	if (isStarting() || isShutDown()) {
		m_power_kW = 0;
	}
	// Conditions for shutting down if going below min turndown
	else if (m_power_kW < m_unitPowerMin_kW && m_hoursSinceStart > m_startup_hours + m_hoursRampUp) {

		if (m_shutdownOption == FuelCell::FC_SHUTDOWN_OPTION::IDLE) {
			m_power_kW = m_unitPowerMin_kW;
		}
		else {
			m_startedUp = false;
			m_shuttingDown = true;
			m_hoursSinceStart = 0;
			m_power_kW = 0;
		}
		
	}
	else if (isRunning()) {
		m_power_kW = fmax(m_power_kW, m_unitPowerMin_kW);
	}
}
void FuelCell::checkMaxLimit() {
	m_power_kW = fmin(m_power_kW, m_unitPowerMax_kW);
}

void FuelCell::checkAvailableFuel() {
	m_availableFuel_MCf -= m_fuelConsumed_MCf;

	if (m_availableFuel_MCf <= 0) {
		m_startedUp = false;
		m_shutDown = true;
		m_shuttingDown = false;
		m_startingUp = false;
		m_hoursSinceStart = 0;
		m_hoursSinceStop = 0;
	}
}

void FuelCell::applyDegradation() {

	if (isRunning() && m_power_kW > 0) {
		m_powerMax_kW -= m_degradation_kWperHour * dt_hour;
		m_power_kW = fmin(m_power_kW, m_powerMax_kW);
	}
	else if (isShuttingDown() && m_powerPrevious_kW > 0) {
		m_powerMax_kW -= m_degradationRestart_kW;
		if (m_powerMax_kW < 0) {
			m_powerMax_kW = 0;
		}
	}

	if (m_replacementOption == FC_REPLACEMENT_OPTION::REPLACE_AT_CAPACITY) {
		if (m_powerMax_kW < m_unitPowerMax_kW * m_replacement_percent) {
			m_powerMax_kW = m_unitPowerMax_kW;
			m_replacementCount += 1;
		}
	}
	else if (m_replacementOption == FC_REPLACEMENT_OPTION::REPLACE_ON_SCHEDULE) {
		int hour = (int)std::floor(m_hour);
		if (hour % 8760 == 0 && m_replacementSchedule[m_year] > 0) {
			m_powerMax_kW = m_unitPowerMax_kW;
			m_replacementCount += 1;
		} 
	}
	// If stack degrades below minimum turndown, shutdown
	if (m_powerMax_kW <= m_unitPowerMin_kW) {
		m_power_kW = 0;
		m_startedUp = false;
		m_shutDown = true;
		m_shuttingDown = false;
		m_hoursSinceStart = 0;
		m_hoursSinceStop = 0;
	}
}

void FuelCell::applyEfficiency() {

	// Shutting down generates heat but no electricity
	if (isShuttingDown()) {
		calculateEfficiencyCurve(0.0);
		m_powerThermal_kW = m_powerMax_kW * m_heatRecovery_percent;
	}
	// When completely shut down, no heat, no electricity
	else if (isShutDown()) {
		m_powerThermal_kW = 0.0;
		m_fuelConsumed_MCf = 0;
	}
	else {
		calculateEfficiencyCurve(getPercentLoad());
		m_powerThermal_kW = m_power_kW;
		m_powerThermal_kW *= m_heatRecovery_percent;
	}
}

void FuelCell::calculateTime() {
	
	m_hour += dt_hour;
	int hour = (int)std::floor(m_hour);

	if (hour % 8760 == 0) {
		m_year++;
	}
}


void FuelCell::runSingleTimeStep(double power_kW) {

	m_powerPrevious_kW = m_power_kW;

	checkStatus(power_kW);

	// Check dynamic response limits
	if (isRunning()) {
		checkPowerResponse();
	}

	// Check minimum power, maximum power
	checkMinTurndown();
	checkMaxLimit();

	// Calculate electrical and thermal efficiency
	applyEfficiency();

	// Ensure there is adequate fuel to generate this time step
	checkAvailableFuel();
	
	// Update internal clock
	calculateTime();


	applyDegradation();
	
}