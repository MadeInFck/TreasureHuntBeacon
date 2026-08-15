# Configuration

[English](CONFIGURATION.md) · **Français**

Tout ce qui se modifie se trouve en tête de `beacon/beacon.ino`.

## Réglages propres à chaque balise

| Constante | Signification |
|---|---|
| `STEP_ID` | Numéro d'étape, octet haut du MINOR. **À changer pour chaque balise.** |
| `HUNT_ID` | Numéro de chasse, MAJOR. Identique pour toutes les balises d'une chasse. |
| `TX_POWER_1M` | RSSI à 1 m. **À mesurer par exemplaire** — voir [calibration](../docs/calibration.fr.md). |

Flashez le même fichier une fois par étape, en ne changeant que `STEP_ID`.

Étiquetez physiquement chaque carte au moment du flash. Deux cartes identiques
sont impossibles à distinguer ensuite, et un RSSI attribué à la mauvaise balise
fausse toute la comparaison.

## Fonctions optionnelles

| Drapeau | Défaut | Nécessite |
|---|---|---|
| `ENABLE_BATTERY_TELEMETRY` | `0` | Un pont diviseur — voir [matériel](../hardware/README.fr.md) |
| `ENABLE_STATUS_LED` | `1` | Une LED sur la carte |

Télémétrie désactivée, l'octet bas du MINOR vaut `0x00`, ce que la
spécification définit comme « pas de télémétrie ». Ne l'activez qu'une fois le
pont diviseur câblé, sinon l'application affichera un niveau de batterie faux.

## Broche ADC pour la télémétrie batterie

Réglez `BAT_ADC_PIN` sur une broche de l'**ADC1**. L'ADC2 est inutilisable
pendant que la radio fonctionne sur plusieurs variantes d'ESP32.

| Carte | Broche conseillée | Remarque |
|---|---|---|
| M5Stack NanoC6 | `1` | Fil blanc du Grove ; seuls G1 et G2 sont exposés |
| XIAO ESP32C6 | `0` (A0) | GPIO3 et GPIO14 réservés au commutateur RF |
| XIAO ESP32C3 | `3` (A1) | **Évitez A0/GPIO2**, broche de strapping |
| ESP32 classique | `32`–`39` | Plage ADC1 |

Calibrez `BAT_CAL` une fois par carte : mesurez la tension réelle de la cellule
au multimètre, divisez par ce qu'affiche la console série, écrivez le quotient.
Deux résistances à 1 % introduisent à elles seules jusqu'à 2 % d'erreur.

## LED de statut

`LED_BUILTIN` est définie par le core pour les cartes reconnues. Si la LED reste
éteinte, vérifiez le brochage de votre carte et indiquez le numéro
explicitement. Si elle reste allumée et clignote en s'éteignant, passez
`LED_ACTIVE_HIGH` à `0`.

Retirez ou désactivez la LED avant de dissimuler une balise en extérieur. Une
boîte qui clignote sous un rocher est un indice que vous n'aviez pas prévu de
donner.

## IDE Arduino

- Core ESP32 **3.x** requis pour les cibles C6 et H2
- Sélectionnez la carte exacte dans le menu Outils
- Moniteur série à **115200**
- Sur les cartes en USB natif, le port disparaît et se recrée à chaque reset ;
  double appui sur RESET pour forcer le bootloader s'il n'apparaît pas
