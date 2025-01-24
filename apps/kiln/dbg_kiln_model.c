#include "dbg_kiln_model.h"
#include "minio.h"

static struct {
    float temp;
    float temp_ambient;
    float energy; // wattsecond
    float C_energy_heat_transfer;
    float C_energy_to_heat;
    float C_heat_dissipation;
} model;

float model_tick(float power, float dt) {
    float dtemp = model.temp - model.temp_ambient;
    model.energy += power * dt;
    float energy_heat = model.energy * model.C_energy_heat_transfer;
    model.energy -= energy_heat;

    model.temp += energy_heat * model.C_energy_to_heat;
    // dissipation
    model.temp -= dtemp * model.C_heat_dissipation;

    return model.temp;
}

float model_temp(void) {
    return model.temp;
}

void model_init(void) {
    model.temp = model.temp_ambient = 20;
    model.energy = 0;

    model.C_energy_heat_transfer = 0.3;
    model.C_energy_to_heat = 0.94;
    model.C_heat_dissipation = 0.003;
}
