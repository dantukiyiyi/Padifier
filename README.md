# Padifier

Plugin VST3 para Windows (uso personal) pensado para FL Studio.

## Instalación

1. Ir a la pestaña **Releases** de este repositorio.
2. Descargar `Padifier-Windows-VST3.zip` de la última versión.
3. Descomprimir y copiar la carpeta `Padifier.vst3` en `C:\Program Files\Common Files\VST3\`.
4. En FL Studio, ir a Opciones > Administrar plugins > Buscar nuevos plugins.

## Controles

- **Armonías**: Mezcla. Sub-controles: Sensibilidad, High End, Low End.
- **Elasticidad**: Mezcla. Sub-controles: High End, Low End.
- **Reverb**: Mezcla (controla el decaimiento). Sub-controles: High End, Low End.
- **Distancia**: Mezcla. Sub-controles: High End, Low End.
- **Respiraciones**: Mezcla. Sub-controles: High End, Low End.
- **Stereo**: -100% (mono) a 0% (original) a +100% (expansión). Sin sub-controles.
- **Saturación**: Mezcla. Sub-controles: Agresividad, High End, Low End.
- **Flanger**: Mezcla. Sub-controles: Rate, Profundidad, Feedback, High End, Low End.
- **Master**: Filtro High End y Filtro Low End sobre la salida final.

Los sub-controles se despliegan con el botón "Ajustes" de cada módulo.

## Notas de la versión 1

Esta primera versión usa procesamiento en el dominio del tiempo para que compile y funcione de forma estable. Algunas partes son simplificaciones:

- **Armonías**: quinta y octava generadas con un pitch shifter de líneas de retardo. La sensibilidad se basa en la envolvente de entrada, no en el análisis espectral de picos.
- **Elasticidad**: difusión con 4 filtros allpass, activa solo mientras entra señal.
- **Reverb**: cola de filtros comb, sin compuerta, con decaimiento de 5 ms a unos 3 s.
- **Stereo**: procesamiento mid/side simple.
- **Saturación**: armónicos sintéticos generados a partir de la señal de referencia, sin modificar la onda original.

## Compilación

Se compila automáticamente en GitHub Actions (runner de Windows) con JUCE 8.0.4. Cada push a `main` crea una nueva Release con el zip listo para descargar.
