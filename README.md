# Nuvio Native Legacy (Versión Personal)

> ### Aviso de Proyecto Personal y Derechos de Autor
>
> Este repositorio contiene una versión personalizada, independiente y no oficial mantenida por **Juansher** (`AlessandroA586`) con fines de prueba, optimización y uso privado en televisores propios (Samsung Smart TV y Android TV).
>
> **Créditos y Reconocimientos:**
> - **iqui27**: Creador y programador principal de la versión nativa en C y SDL2 para televisores inteligentes.
> - **NuvioMedia**: Creadores originales de la plataforma y del concepto oficial de Nuvio.
>
> Todos los derechos, marcas registradas, nombres y logotipos pertenecen a sus respectivos autores. Este proyecto respeta la licencia de código abierto GNU General Public License v3.0 (GPLv3).

---

## Sobre este Proyecto

Esta aplicación es una adaptación de alto rendimiento del reproductor Nuvio para televisores inteligentes. Está programada en lenguaje C con aceleración gráfica directa para lograr la máxima fluidez, navegación rápida y reproducción estable en pantallas grandes.

### Sistemas Compatibles

- **Samsung Smart TV (Tizen OS)**: Compatible con televisores Samsung desde 2018 en adelante (Tizen 4.0 hasta Tizen 9.0), mediante paquetes `.tpk`.
- **Android TV y Google TV**: Compatible con televisores, cajas TV Box y dispositivos con Android 7.0 o superior, mediante archivo `.apk`.

---

## Cómo Instalar la Aplicación

### 1. En televisores Samsung (Tizen)

1. En la tienda de aplicaciones de la tele (Apps), presiona los números `1 2 3 4 5` en el control remoto para activar el modo de desarrollo (**Developer Mode**).
2. Activa la casilla **ON**, escribe la dirección IP de tu computadora y reinicia el televisor.
3. Abre el programa [Apps2Samsung](https://github.com/Apps2Samsung/Apps2Samsung) en tu computadora.
4. Conéctate a la dirección IP de tu televisor y selecciona el archivo `.tpk` correspondiente al año de tu tele para instalarlo.

### 2. En televisores Android TV

1. Descarga el archivo de instalación `.apk`.
2. Pásalo a una memoria USB para instalarlo en la tele, o instálalo directamente desde tu computadora usando el comando ADB:
   ```bash
   adb connect <IP_DE_TU_TELEVISOR>:5555
   adb install -r Nuvio-android.apk
   ```

---

## Proyectos Originales

Para conocer los proyectos base o contribuir con el desarrollo general:
- Repositorio de la versión nativa en C: [iqui27/nuvio-native-legacy](https://github.com/iqui27/nuvio-native-legacy)
- Proyecto oficial de Nuvio: [NuvioMedia](https://github.com/NuvioMedia)

---

## Licencia

Este proyecto se comparte bajo los términos de la Licencia Pública General de GNU versión 3.0 (GPLv3). Consulta el archivo [LICENSE](LICENSE) para más detalles.
