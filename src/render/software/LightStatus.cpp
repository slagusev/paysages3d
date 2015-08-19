#include "LightStatus.h"

#include "LightingManager.h"
#include "Color.h"
#include "SurfaceMaterial.h"

LightStatus::LightStatus(LightingManager *manager, const Vector3 &location, const Vector3 &eye)
{
    this->max_power = 0.0;
    this->manager = manager;
    this->location = location;
    this->eye = eye;
}

void LightStatus::pushComponent(LightComponent component)
{
    double power = component.color.getPower();
    if (component.altered && (power < max_power * 0.05 || power < 0.001))
    {
        // Exclude filtered lights that are owerpowered by a previous one
        return;
    }

    if (manager->alterLight(component, location))
    {
        if (power > max_power)
        {
            max_power = power;
        }
        components.push_back(component);
    }
}

Color LightStatus::apply(const Vector3 &normal, const SurfaceMaterial &material)
{
    Color final(0.0, 0.0, 0.0, 0.0);

    for (auto component: components)
    {
        final = final.add(manager->applyFinalComponent(component, eye, location, normal, material));
    }

    final.a = material.base->a;
    return final;
}
