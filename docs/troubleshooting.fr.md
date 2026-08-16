# Dépannage

[English](troubleshooting.md) · **Français**

## nRF Connect sur iPhone n'affiche rien

**C'est le comportement attendu. Ce n'est pas un défaut de votre balise.**

iOS filtre les données constructeur Apple (`0x004C`) dans CoreBluetooth. Toute
application iOS scannant de cette manière ne voit qu'un paquet vide : ni UUID,
ni major, ni minor, ni badge iBeacon. La même balise affiche tout dans nRF
Connect pour Android.

Sur iPhone, **CoreLocation est le seul moyen de voir un iBeacon**. N'utilisez
pas une application de scan iOS pour déboguer une balise.

Pour inspecter les trames brutes, utilisez l'une de ces options :

- **Un Mac.** Les Mac n'appliquent pas le même filtre. LightBlue fait l'affaire.
- **`firmware/scanner`** sur un second ESP32.
- **Un téléphone Android** avec nRF Connect.

## Rien n'est détecté, par ordre de probabilité

**1. UUID différent.** De loin la cause la plus fréquente, et l'échec est
silencieux : CoreLocation ne remonte rien plutôt qu'une erreur. Comparez l'UUID
de l'application aux octets 4 à 19 de la trame émise, dans cet ordre exact.

**2. Clé Info.plist manquante.** `NSLocationWhenInUseUsageDescription` doit être
présente. Sans elle, iOS refuse en silence : pas de crash, pas d'erreur, rien.

**3. Autorisation non accordée.** `requestWhenInUseAuthorization()` doit être
appelée et la boîte de dialogue acceptée. Vérifiez dans Réglages → votre
application → Position.

**4. Monitoring au lieu de ranging.** Ce sont deux API distinctes.
`startRangingBeacons(satisfying:)` est celle qui donne RSSI et distance.

**5. CLLocationManager non retenu.** Un manager déclaré en variable locale est
désalloué à la sortie de la fonction, et le ranging s'arrête sans le moindre
message. Conservez-le dans une propriété.

**6. Filtrage sur un minor complet.** Contraignez sur UUID et major seulement.
L'octet bas du minor porte le niveau de batterie et change au fil de la
décharge : un filtre sur le minor entier cessera de correspondre.

## Vérifier le côté balise

Ouvrez le moniteur série à 115200. Le firmware imprime la trame avant de
démarrer l'advertising :

```
AD mfg (25 bytes): 1A FF 4C 00 02 15 A4 C2 ...
Advertising total: 30 / 31 bytes
Advertising: YES
```

Si `4C 00 02 15` suit `1A FF`, la trame est correcte. Si `Advertising` indique
`NO`, la pile a refusé de démarrer et le problème est sur la carte.

Si le dump est correct **et** l'advertising actif, la balise est presque
certainement bonne et le problème se situe dans l'application.

## Erreurs de compilation

### `'ADV_TYPE_NONCONN_IND' was not declared in this scope`

Cette constante appartient à Bluedroid. Sur les cibles NimBLE comme l'ESP32-C6,
elle n'existe pas. Supprimez la ligne `setAdvertisementType()` — le firmware
fourni l'omet déjà.

La balise émet alors en connectable plutôt qu'en non-connectable. La
spécification d'Apple prévoit le non-connectable, mais CoreLocation range les
deux de façon identique et des balises du commerce émettent en connectable.

### Carte absente de la liste

Les cibles C6 et H2 nécessitent le core ESP32 **3.x**. Mettez le core à jour,
plutôt que de chercher dans la liste.

### Pas de port série

Les cartes en USB natif ne s'énumèrent qu'une fois démarrées, et le port
disparaît à chaque reset. Double appui sur RESET pour forcer le bootloader.
Vérifiez que le câble est bien un câble de données — les câbles USB de charge
seule sont le piège classique.

### Tout compile mais le moniteur série devient muet

Arduino-ESP32 compile les cibles RISC-V avec **newlib-nano**, qui n'implémente
pas le formatage des flottants. Un seul `%f` dans un `Serial.printf` peut
avaler la ligne entière ou afficher n'importe quoi — et le symptôme ressemble à
une carte plantée, pas à un problème de formatage.

C'est pourquoi le firmware fourni travaille exclusivement en entiers :
`DIVIDER_X100` et `BAT_CAL_X1000` sont des entiers mis à l'échelle, affichés
par division et modulo. Conservez ce principe.

### `redefinition of 'setup()'` et une dizaine d'erreurs semblables

L'IDE Arduino concatène **tous les `.ino` d'un dossier de croquis** avant de
compiler. Deux variantes de firmware côte à côte, c'est deux `setup()`, deux
`BEACON_UUID`, et ainsi de suite.

Un croquis par dossier, et le nom du dossier doit correspondre à celui du
`.ino`. Profitez-en pour éviter les espaces dans les noms.

## Les distances sont fausses

Systématiquement **trop courtes** : `TX_POWER_1M` est trop bas. Systématiquement
trop longues : il est trop haut. N'ajustez pas à tâtons, mesurez à 1 m, voir
[calibration](calibration.fr.md).

Des erreurs qui grandissent avec la distance sont inhérentes au modèle
logarithmique, pas un défaut de calibration. Au-delà de 5 m, une estimation
iBeacon est au mieux indicative.
