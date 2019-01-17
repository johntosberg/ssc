#ifndef lib_geothermal_test_h_
#define lib_geothermal_test_h_
#include <gtest/gtest.h>
#include "lib_geothermal.h"
#include "lib_weatherfile.h"
#include "lib_physics.h"
#include "lib_powerblock.h"


class CGeothermalAnalyzerBinary : public ::testing::Test
{	
protected:

	//Inputs used for calculating values of struct members. These are the current defaults in SAM 2018.11.11:
	int	well_flow_rate;
	double num_wells_getem;
	int nameplate;
	int analysis_type;
	int conversion_type;
	int conversion_subtype;
	int plant_efficiency_input;
	int decline_type;
	double temp_decline_rate;
	int temp_decline_max;
	int	wet_bulb_temp;
	double ambient_pressure;
	int pump_efficiency;
	int delta_pressure_equip;
	double excess_pressure_pump;
	int well_diameter;
	double casing_size;
	int inj_well_diam;
	int specify_pump_work;
	int specified_pump_work_amount;
	int	resource_type;
	int resource_depth;
	int resource_temp;
	int design_temp = 0;
	int rock_thermal_conductivity;
	int rock_specific_heat;
	int rock_density;
	double reservoir_pressure_change;
	int reservoir_width;
	int	reservoir_pressure_change_type;
	double reservoir_height;
	double reservoir_permeability;
	int  inj_prod_well_distance;
	int subsurface_water_loss;
	double fracture_aperature;
	int num_fractures;
	int fracture_width;
	int fracture_angle;
	int geothermal_analysis_period;
	int resource_potential;
	char file_name[300];
	int tou[8760];
			  
	//Initializing structs to defualt values in SAM 2018.11.11:
		SPowerBlockParameters SPBP;
		SPowerBlockInputs PBInputs;
		SGeothermal_Inputs geoBinary_inputs;
		SGeothermal_Outputs geoBinary_outputs;
	
	//Constructing CGeothermalAnalyzer class for testing:
	CGeothermalAnalyzer* binaryDefault; 
	
public:
	void SetUp() {
		well_flow_rate = 110,
		num_wells_getem = 4.31975,
		nameplate = 15000,
		analysis_type = 0,
		conversion_type = BINARY,
		conversion_subtype = 0,
		plant_efficiency_input = 80,
		decline_type = 0,
		temp_decline_rate = 0.3,
		temp_decline_max = 30,
		wet_bulb_temp = 15,
		ambient_pressure = 14.7,
		pump_efficiency = 60,
		delta_pressure_equip = 25,
		excess_pressure_pump = 50.76,
		well_diameter = 10,
		casing_size = 9.625,
		inj_well_diam = 10,
		specify_pump_work = 0,
		specified_pump_work_amount = 0,
		resource_type = 0,
		resource_depth = 6000,
		resource_temp = 200,
		design_temp = 0,
		rock_thermal_conductivity = 259200,
		rock_specific_heat = 950,
		rock_density = 2600,
		reservoir_pressure_change = 0.35,
		reservoir_width = 500,
		reservoir_pressure_change_type = 0,
		reservoir_height = 100,
		reservoir_permeability = 0.05,
		inj_prod_well_distance = 1500,
		subsurface_water_loss = 2,
		fracture_aperature = 0.0004,
		num_fractures = 6,
		fracture_width = 175,
		fracture_angle = 15,
		geothermal_analysis_period = 30,
		resource_potential = 210;
		//file_name = '%s/test/input_cases/general_data/daggett_ca_34.865371_-116.783023_psmv3_60_tmy.csv';
		

		//====================================================================================================================================================================
		//SPowerBlockParameters SPBP;
		SPBP.tech_type = 4;
		SPBP.T_htf_cold_ref = 90;						// design outlet fluid temp
		SPBP.T_htf_hot_ref = 175;						// design inlet fluid temp
		SPBP.HTF = 3;									// heat transfer fluid type - set in interface, but no user input
		SPBP.P_ref = nameplate / 1000;					// P_ref wants MW, 'nameplate' in kW
		SPBP.P_boil = 2;
		SPBP.eta_ref = 0.17;
		SPBP.q_sby_frac = 0.2;
		SPBP.startup_frac = 0.2;
		SPBP.startup_time = 1;
		SPBP.pb_bd_frac = 0.013;
		SPBP.T_amb_des = 27;
		SPBP.CT = 0;
		SPBP.dT_cw_ref = 10;
		SPBP.T_approach = 5;
		SPBP.T_ITD_des = 16;
		SPBP.P_cond_ratio = 1.0028;
		SPBP.P_cond_min = 1.25;
		SPBP.n_pl_inc = 8;
		SPBP.F_wc[0] = 0;
		SPBP.F_wc[1] = 0;
		SPBP.F_wc[2] = 0;
		SPBP.F_wc[3] = 0;
		SPBP.F_wc[4] = 0;
		SPBP.F_wc[5] = 0;
		SPBP.F_wc[6] = 0;
		SPBP.F_wc[7] = 0;
		SPBP.F_wc[8] = 0;
		
		//====================================================================================================================================================================
		//SPowerBlockInputs PBInputs;
		PBInputs.mode = 2;
		if (true) // used number of wells as calculated by GETEM
			PBInputs.m_dot_htf = well_flow_rate * 3600.0 * num_wells_getem;
		//Ignoring user defined number of wells for now.

		PBInputs.demand_var = PBInputs.m_dot_htf;
		PBInputs.standby_control = 1;
		PBInputs.rel_humidity = 0.7;

		//====================================================================================================================================================================
		//SGeothermal_Inputs geoBinary_inputs;
		geoBinary_inputs.md_RatioInjectionToProduction = 0.5;
		geoBinary_inputs.md_DesiredSalesCapacityKW = nameplate;

		//geoBinary_inputs.md_NumberOfWells = as_double("num_wells");

		if (analysis_type == 0)
			geoBinary_inputs.me_cb = POWER_SALES;
		else
			geoBinary_inputs.me_cb = NUMBER_OF_WELLS;

		if (conversion_type == 0)
			geoBinary_inputs.me_ct = BINARY;
		else if (conversion_type == 1)
			geoBinary_inputs.me_ct = FLASH;

		switch (conversion_subtype)
		{
		case 0:	geoBinary_inputs.me_ft = SINGLE_FLASH_NO_TEMP_CONSTRAINT; break;
		case 1:	geoBinary_inputs.me_ft = SINGLE_FLASH_WITH_TEMP_CONSTRAINT; break;
		case 2:	geoBinary_inputs.me_ft = DUAL_FLASH_NO_TEMP_CONSTRAINT; break;
		case 3:	geoBinary_inputs.me_ft = DUAL_FLASH_WITH_TEMP_CONSTRAINT; break;
		}
		geoBinary_inputs.md_PlantEfficiency = plant_efficiency_input / 100;

		// temperature decline
		if (decline_type == 0)
			geoBinary_inputs.me_tdm = ENTER_RATE;
		else if (decline_type == 1)
			geoBinary_inputs.me_tdm = CALCULATE_RATE;
		geoBinary_inputs.md_TemperatureDeclineRate = temp_decline_rate / 100;
		geoBinary_inputs.md_MaxTempDeclineC = temp_decline_max;

		// flash inputs
		geoBinary_inputs.md_TemperatureWetBulbC = wet_bulb_temp;
		geoBinary_inputs.md_PressureAmbientPSI = ambient_pressure;

		//pumping parameters
		geoBinary_inputs.md_ProductionFlowRateKgPerS = well_flow_rate;
		geoBinary_inputs.md_GFPumpEfficiency = pump_efficiency / 100;
		geoBinary_inputs.md_PressureChangeAcrossSurfaceEquipmentPSI = delta_pressure_equip;
		geoBinary_inputs.md_ExcessPressureBar = physics::PsiToBar(excess_pressure_pump);
		geoBinary_inputs.md_DiameterProductionWellInches = well_diameter;
		geoBinary_inputs.md_DiameterPumpCasingInches = casing_size;
		geoBinary_inputs.md_DiameterInjectionWellInches = inj_well_diam;
		geoBinary_inputs.mb_CalculatePumpWork = (1 != specify_pump_work);
		geoBinary_inputs.md_UserSpecifiedPumpWorkKW = specified_pump_work_amount * 1000; // entered in MW

		//resource characterization
		if (resource_type == 0)
			geoBinary_inputs.me_rt = HYDROTHERMAL;
		else if (resource_type == 1)
			geoBinary_inputs.me_rt = EGS;
		geoBinary_inputs.md_ResourceDepthM = resource_depth;
		geoBinary_inputs.md_TemperatureResourceC = resource_temp;
		geoBinary_inputs.me_dc = TEMPERATURE;
		geoBinary_inputs.md_TemperaturePlantDesignC = design_temp;



		//reservoir properties
		geoBinary_inputs.md_TemperatureEGSAmbientC = 15.0;
		geoBinary_inputs.md_EGSThermalConductivity = rock_thermal_conductivity;
		geoBinary_inputs.md_EGSSpecificHeatConstant = rock_specific_heat;
		geoBinary_inputs.md_EGSRockDensity = rock_density;
		switch (reservoir_pressure_change_type)
		{
		case 0: geoBinary_inputs.me_pc = ENTER_PC; break;				// pressure change entered by user
		case 1: geoBinary_inputs.me_pc = SIMPLE_FRACTURE; break;		// use fracture flow (EGS only)
		case 2: geoBinary_inputs.me_pc = K_AREA; break;				// permeability * area
		}
		geoBinary_inputs.md_ReservoirDeltaPressure = reservoir_pressure_change;
		geoBinary_inputs.md_ReservoirWidthM = reservoir_width;
		geoBinary_inputs.md_ReservoirHeightM = reservoir_height;
		geoBinary_inputs.md_ReservoirPermeability = reservoir_permeability;
		geoBinary_inputs.md_DistanceBetweenProductionInjectionWellsM = inj_prod_well_distance;
		geoBinary_inputs.md_WaterLossPercent = subsurface_water_loss / 100;
		geoBinary_inputs.md_EGSFractureAperature = fracture_aperature;
		geoBinary_inputs.md_EGSNumberOfFractures = num_fractures;
		geoBinary_inputs.md_EGSFractureWidthM = fracture_width;
		geoBinary_inputs.md_EGSFractureAngle = fracture_angle;

		// calculate output array sizes
		geoBinary_inputs.mi_ModelChoice = 0;		 // 0=GETEM, 1=Power Block monthly, 2=Power Block hourly
		// set geothermal inputs RE how analysis is done and for how long
		geoBinary_inputs.mi_ProjectLifeYears = geothermal_analysis_period;
		//if (geoBinary_inputs.mi_ProjectLifeYears == 0)
		//	throw general_error("invalid analysis period specified in the geothermal hourly model");

		geoBinary_inputs.md_PotentialResourceMW = resource_potential;
		geoBinary_inputs.mc_WeatherFileName = file_name;
		geoBinary_inputs.mia_tou = tou;
		geoBinary_inputs.mi_MakeupCalculationsPerYear = (geoBinary_inputs.mi_ModelChoice == 2) ? 8760 : 12;
		geoBinary_inputs.mi_TotalMakeupCalculations = geoBinary_inputs.mi_ProjectLifeYears * geoBinary_inputs.mi_MakeupCalculationsPerYear;

		//====================================================================================================================================================================
		//SGeothermal_Outputs geoBinary_outputs;

		geoBinary_outputs.maf_ReplacementsByYear = geoBinary_inputs.mi_ProjectLifeYears;
		//ssc_number_t *annual_replacements = allocate( "annual_replacements", geoBinary_inputs.mi_ProjectLifeYears);

		// allocate lifetime monthly arrays (one element per month, over lifetime of project)
		geoBinary_outputs.maf_monthly_resource_temp = 12 * geoBinary_inputs.mi_ProjectLifeYears;
		geoBinary_outputs.maf_monthly_power = 12 * geoBinary_inputs.mi_ProjectLifeYears;
		geoBinary_outputs.maf_monthly_energy = 12 * geoBinary_inputs.mi_ProjectLifeYears;

		// allocate lifetime timestep arrays (one element per timestep, over lifetime of project)
		// if this is a monthly analysis, these are redundant with monthly arrays that track same outputs
		geoBinary_inputs.mi_MakeupCalculationsPerYear = (geoBinary_inputs.mi_ModelChoice == 2) ? 8760 : 12;
		geoBinary_inputs.mi_TotalMakeupCalculations = geoBinary_inputs.mi_ProjectLifeYears * geoBinary_inputs.mi_MakeupCalculationsPerYear;

		geoBinary_outputs.maf_timestep_resource_temp = geoBinary_inputs.mi_TotalMakeupCalculations;
		geoBinary_outputs.maf_timestep_power = geoBinary_inputs.mi_TotalMakeupCalculations;
		geoBinary_outputs.maf_timestep_test_values = geoBinary_inputs.mi_TotalMakeupCalculations;

		geoBinary_outputs.maf_timestep_pressure = geoBinary_inputs.mi_TotalMakeupCalculations;
		geoBinary_outputs.maf_timestep_dry_bulb = geoBinary_inputs.mi_TotalMakeupCalculations;
		geoBinary_outputs.maf_timestep_wet_bulb = geoBinary_inputs.mi_TotalMakeupCalculations;

		geoBinary_outputs.maf_hourly_power = geoBinary_inputs.mi_ProjectLifeYears * 8760;

		//====================================================================================================================================================================
		
		binaryDefault = new CGeothermalAnalyzer(SPBP, PBInputs, geoBinary_inputs, geoBinary_outputs);
	}

	void TearDown() {
		delete binaryDefault;
	}

};

#endif