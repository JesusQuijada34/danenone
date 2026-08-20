# Privacidad de Cloud Danenone Devices

El cliente DaneDesk deriva una huella SHA-256 a partir de identificadores
locales del sistema. No envía los valores de origen, no recoge coordenadas y no
incluye un rastreador silencioso.

La protección de ubicación, cuando se implemente en el servicio propietario,
debe limitarse a dispositivos reportados como perdidos y a propietarios
autenticados. El cliente local debe informar de forma visible el estado de
bloqueo y conservar una vía de recuperación verificable mediante OTP.

El paquete sólo admite endpoints HTTPS para evitar que los códigos de activación
o las respuestas de estado se intercambien sobre conexiones sin cifrado.
