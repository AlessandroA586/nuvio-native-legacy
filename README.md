# Nuvio Native Legacy (Bifurcación Personal)

> ### Aviso de Proyecto Personal y Reconocimiento de Derechos
>
> Este repositorio constituye una bifurcación (fork) de carácter estrictamente personal, independiente y no oficial, mantenida por **Juansher** (`AlessandroA586`) con fines de experimentación, optimización y uso privado en dispositivos propios (Samsung Tizen y Android TV).
>
> **Créditos y Atribución:**
> - **iqui27**: Creador y desarrollador principal del proyecto `nuvio-native-legacy`, responsable de la reimplementación técnica del cliente en C, SDL2 y su arquitectura nativa para plataformas de televisión.
> - **NuvioMedia**: Creadores y autores originales de la plataforma, diseño y ecosistema oficial de Nuvio.
>
> Todos los derechos de autor, marcas comerciales, nombres de productos y logotipos pertenecen exclusivamente a sus respectivos creadores originales. Este proyecto opera en estricta conformidad con la Licencia Pública General de GNU v3.0 (GPLv3).

---

## Descripción del Proyecto

Esta versión corresponde a una adaptación nativa de alto rendimiento del cliente Nuvio para televisores inteligentes, desarrollada en lenguaje C con aceleración gráfica por hardware (OpenGL ES y SDL2) para garantizar una interfaz fluida y una respuesta inmediata de reproducción.

### Plataformas Soportadas

- **Samsung Smart TV (Tizen OS)**: Compatible con modelos desde 2018 en adelante (Tizen 4.0 hasta Tizen 9.0) mediante paquetes nativos `.tpk`.
- **Android TV y Google TV**: Compatible con televisores, TV Boxes y dispositivos con Android 7.0 o superior mediante paquete `.apk`.

---

## Instalación

### 1. Samsung Smart TV (Tizen OS)

1. En la sección de aplicaciones del televisor (Apps), presione secuencialmente los botones `1 2 3 4 5` en el control remoto para abrir el **Developer Mode**.
2. Active la opción **ON** e introduzca la dirección IP local de su computadora. Reinicie el televisor.
3. Desde su equipo personal, ejecute la utilidad [Apps2Samsung](https://github.com/Apps2Samsung/Apps2Samsung).
4. Conéctese a la dirección IP del televisor y seleccione el paquete `.tpk` correspondiente a la versión de Tizen de su pantalla.

### 2. Android TV

1. Descargue el paquete de instalación `.apk` correspondiente a la versión deseada.
2. Transfiera el archivo al dispositivo mediante almacenamiento USB o instálelo directamente vía red local mediante ADB:
   ```bash
   adb connect <DIRECCION_IP_DEL_TELEVISOR>:5555
   adb install -r Nuvio-android.apk
   ```

---

## Referencias y Proyectos Originales

Para consultar la base de código central o reportar incidencias generales de la arquitectura upstream, diríjase a los repositorios originales:
- Base de implementación nativa en C: [iqui27/nuvio-native-legacy](https://github.com/iqui27/nuvio-native-legacy)
- Plataforma oficial de Nuvio: [NuvioMedia](https://github.com/NuvioMedia)

---

## Licencia

Este proyecto se distribuye bajo los términos de la Licencia Pública General de GNU versión 3.0 (GPLv3). Para mayores detalles, consulte el archivo [LICENSE](LICENSE).
