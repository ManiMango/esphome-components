import esphome.codegen as cg
from esphome.components import light, output
from esphome.components.light.effects import register_monochromatic_effect
import esphome.config_validation as cv
from esphome.const import CONF_EFFECTS, CONF_OUTPUT_ID
from esphome.types import ConfigType

CONF_PERIOD = "period"
CONF_PIN_A = "pin_a"
CONF_PIN_B = "pin_b"

mono_hbridge_ns = cg.esphome_ns.namespace("mono_hbridge")
MonoHBridgeOutput = mono_hbridge_ns.class_("MonoHBridgeOutput", light.LightOutput)
LightEffect = light.light_ns.class_("LightEffect")
PolarityEffect = mono_hbridge_ns.class_("PolarityEffect", LightEffect)
PolarityPattern = mono_hbridge_ns.enum("PolarityPattern", True)

# effect key, default name, pattern, default period
EFFECTS = (
    ("polarity_alternate", "Alternate", "ALTERNATE", "1s"),
    ("polarity_crossfade", "Cross Fade", "CROSSFADE", "4s"),
    ("polarity_blink", "Blink", "BLINK", "1s"),
    ("polarity_breathe", "Breathe", "BREATHE", "4s"),
    ("polarity_strobe_alternate", "Strobe Alternate", "STROBE_ALTERNATE", "400ms"),
    ("polarity_blink_a", "Blink A", "BLINK_A", "1s"),
    ("polarity_blink_b", "Blink B", "BLINK_B", "1s"),
    ("polarity_only_a", "Polarity A Only", "ONLY_A", "1s"),
    ("polarity_only_b", "Polarity B Only", "ONLY_B", "1s"),
)


def _register_effect(key: str, name: str, pattern: str, period: str) -> None:
    @register_monochromatic_effect(
        key,
        PolarityEffect,
        name,
        {
            cv.Optional(
                CONF_PERIOD, default=period
            ): cv.positive_time_period_milliseconds,
        },
    )
    async def effect_to_code(config: ConfigType, effect_id):
        effect = cg.new_Pvariable(
            effect_id,
            config["name"],
            getattr(PolarityPattern, f"POLARITY_PATTERN_{pattern}"),
        )
        cg.add(effect.set_period(config[CONF_PERIOD]))
        return effect


for _effect in EFFECTS:
    _register_effect(*_effect)


def _add_default_effects(config: ConfigType) -> ConfigType:
    """Offer every polarity effect when the user lists none."""
    if CONF_EFFECTS not in config:
        config = {**config, CONF_EFFECTS: [{key: {}} for key, *_ in EFFECTS]}
    return config


CONFIG_SCHEMA = cv.All(
    _add_default_effects,
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MonoHBridgeOutput),
            cv.Required(CONF_PIN_A): cv.use_id(output.FloatOutput),
            cv.Required(CONF_PIN_B): cv.use_id(output.FloatOutput),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    pin_a = await cg.get_variable(config[CONF_PIN_A])
    cg.add(var.set_pin_a(pin_a))
    pin_b = await cg.get_variable(config[CONF_PIN_B])
    cg.add(var.set_pin_b(pin_b))
