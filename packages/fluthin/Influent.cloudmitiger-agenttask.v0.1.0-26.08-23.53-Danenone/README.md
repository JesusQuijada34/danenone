# Cloud Danenone Devices

**Identidad pública:** `Influent.cloudmitiger-agenttask.v0.1.0-26.08-23.53-Danenone`

**Publisher:** `Influent`

**Autor:** `JesusQuijada34`

**Plataforma:** `Danenone`

Cloud Danenone Devices instala el cliente local DaneDesk para vincular un equipo
Danenone mediante un código alfanumérico temporal de 6 a 12 caracteres. El
cliente consulta el estado recuperable del dispositivo, registra localmente un
bloqueo solicitado por el propietario y permite validar un OTP de seis dígitos
emitido para la recuperación.

## Uso

El comando principal es `cloudmitiger-agenttask`. Sus operaciones se expresan
en JSON para que la pantalla de bloqueo y el OOBE puedan interpretarlas sin
depender de texto localizado.

```bash
cloudmitiger-agenttask activate --server https://foundstore.example --code AB12CD34 --name "Mi Danenone"
cloudmitiger-agenttask check-status
cloudmitiger-agenttask enforce-lock
cloudmitiger-agenttask recover --otp 123456
```

La dirección de Cloud Danenone Devices debe usar HTTPS. El cliente guarda su
configuración con permisos `0600` y sólo transmite una huella SHA-256 de la
identidad local, nunca el identificador crudo. La emisión o verificación de un
OTP no sustituye la autenticación del propietario en Foundstore.

## Privacidad y límites

El paquete no obtiene ni manda coordenadas. La capacidad de protección de
ubicación se administra del lado del propietario autenticado y debe mantener
controles visibles. Los estados `locked` o `lost` son recuperables por OTP
cuando el propietario haya solicitado el código. El cliente no instala
aplicaciones ni acepta órdenes de instalación sin el flujo local de aprobación
de Foundstore.

## Contenido

| Ruta | Finalidad |
| --- | --- |
| `cloudmitiger-agenttask.py` | Punto de entrada del paquete. |
| `app/danedesk_client.py` | Cliente HTTPS, activación, estado, bloqueo y recuperación OTP. |
| `docs/PRIVACY.md` | Garantías y límites de privacidad. |
| `RELEASE_NOTES.md` | Notas que deben usarse como cuerpo del release. |

## Validación

La compilación se realiza mediante el PackageMaker normalizado:

```bash
python3 /home/ubuntu/packagemaker/packagemaker.py --buildthis .
```

Antes de publicar, debe verificarse que el `.iflapp` sea ZIP válido, incluya
`details.xml`, conserve el publisher `Influent` y contenga el ejecutable
Danenone generado.
