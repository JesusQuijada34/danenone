# Recuperación visible de DaneDesk en OOBE

El OOBE consulta Cloud Danenone Devices antes de pasar de su resumen a la instalación cuando `DANEDESK_SERVER` está configurado. La consulta usa exclusivamente la huella SHA-256 local calculada por `danedesk-client`; ni el serial ni el identificador de máquina se transmiten en claro.

| Estado de consulta | Resultado OOBE | Acción permitida |
|---|---|---|
| `active` | Continúa hacia la instalación. | Ninguna recuperación requerida. |
| `lost` o `locked` | Muestra la superficie de recuperación con mensaje del propietario. | Introducir OTP de seis dígitos emitido tras autenticar al propietario en Foundstore. |
| Sin red o error de servicio | No continúa si hay servidor DaneDesk configurado. | Conectarse a una red y repetir la comprobación. |
| Sin `DANEDESK_SERVER` | Conserva el modo instalador local sin servicio cloud configurado. | Continuar; este modo no afirma protección contra reactivación. |

La pantalla de recuperación no borra datos ni intenta controlar firmware. El OTP se verifica con `danedesk-client recover --otp <seis-dígitos>`, que llama a `danedesk.recoverLocal` y vincula el desafío tanto al propósito como a la huella local. Un OTP vence a los cinco minutos, admite cinco intentos y sólo puede utilizarse una vez.

> La protección frente a un borrado completo del disco sigue requiriendo una raíz de confianza en hardware, como Secure Boot administrado, TPM o integración OEM. El OOBE no debe describir esta comprobación de aplicación como una garantía de firmware.
