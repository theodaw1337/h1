# Oplysninger fra brugerens forumindlæg

Gemt 2026-09-28. Kilde: tekst indsat og vedhæftet af brugeren; der er ikke leveret
links til de konkrete indlæg. Oplysningerne er ikke uafhængigt verificeret i spillet.
Den oprindelige guide angiver selv, at offsets blev verificeret 22/9/26.

## Kamera / world-to-screen

Det supplerende indlæg viser C# med en `CameraCache`, der indeholder `Matrix4x4`,
bredde/højde, halve dimensioner og `IsValid`. Hukommelseskæden er:

```text
graphics = ReadPointer(Process.BaseAddress + Offsets.CGraphics)
camera = ReadPointer(graphics + Offsets.Camera)
matrixBase = ReadPointer(camera + Offsets.Matrix)
matrix = Transpose(ReadMatrix(matrixBase + Offsets.ViewProjection))
matrix.M21/M22/M23/M24 *= -1
```

Projektion efter transponering:

```text
w = M41*x + M42*y + M43*z + M44
xClip = M11*x + M12*y + M13*z + M14
yClip = M21*x + M22*y + M23*z + M24
screenX = halfWidth  * (1 + xClip/w)
screenY = halfHeight * (1 - yClip/w)
```

Afvis når `w < NearPlane`. `NearPlane` er ikke defineret i forumuddraget;
den oprindelige C++-bases værdi 0,098 er bevaret.

**Forskel mellem kilder:** C#-uddragets `CGraphics` er et relativt offset, fordi
det lægges til module-base. Den første guides `ADDRESS_CGRAPHICS=0x14476E748`
er angivet som en absolut virtuel adresse. Vores kode bruger den absolutte adresse
uden at lægge module-base til. C# kan ikke indsættes direkte i C++.

Kameraet indlæses én gang pr. billede. Cachen nulstilles før aflæsning, så en
mislykket aflæsning ikke genbruger sidste billedes matrix. Pointerchecks,
finithedskontrol og cachegyldighed er tilføjet til forumuddraget.

## Bullet drop og hastighed

Formlerne fra det supplerende indlæg:

```cpp
float travelSec = distance / bulletSpeed;
predicted.x += velocity.x * travelSec;
predicted.y += velocity.y * travelSec;
predicted.z += velocity.z * travelSec;
float drop = 0.5f * gravity * travelSec * travelSec;
predicted.y += drop;
```

Eksemplet er 200 meter / 375 m/s = ca. 0,533 sekunder, og drop = ca. 0,1422 × g.
Enheder: afstand i meter, hastighed i m/s, tyngdekraft i m/s². Positiv Y antages opad.
Forudsigelsen skal beregnes før projektion til skærmkoordinater.

Den første guides kode indeholdt hastighedsforudsigelse, men **ingen gravity-term**.
Kommentaren om, at den allerede beregnede bullet drop, stemmer derfor ikke med
det indsendte kodeuddrag. Den supplerende formel tilføjer det manglende led.

Ingen kilde oplyser spillets faktiske tyngdekraft eller om den varierer pr. våben.
Derfor er drop valgfrit og deaktiveret, og `Gravity=0` er standard.
Modellen antager konstant projektilhastighed og konstant målhastighed. Den løser
ikke en fuld ballistisk skæringsligning og modellerer ikke luftmodstand.

## To forskellige forslag til det aktive våben

### Metode A: inventory

```cpp
inventory = ReadPointer(localPlayer + 0xCE8);
weaponName = ReadPointer(inventory + 0x24);
// Read a null-terminated asset name at weaponName.
```

### Metode B: weapon container

```cpp
weaponContainer = ReadPointer(localPlayer + 0x688);
activeWeapon = ReadPointer(weaponContainer + 0xB8);
weaponName = ReadPointer(activeWeapon + 0x20);
// Read a null-terminated .adr name at weaponName.
```

Indlægget til metode B siger, at localplayer-adgangen adskiller sig fra andre
spilleres inventory. Vi ved ikke, om kilderne gælder samme spilversion eller
samme type startpointer. `ProfileRPM` og `Memory.Read` er hjælperfunktioner i
forfatternes egne projekter, ikke funktioner, der findes i den leverede C++-base.

Begge metoder er derfor navngivet separat i `Offs.h` og vælges med `Weapon/Path`.
Auto bruger en genkendt metode; ved to forskellige genkendte navne stoppes aim.
Null-terminering og fulde kendte filnavne kontrolleres. Det gør ikke offsets
beviseligt korrekte; navnet skal stadig valideres ved våbenskift i spillet.

## Våbenmodeller og oplyste hastigheder

| Assetnavn | Hastighed, m/s |
|---|---:|
| Weapons_AR15_3P.adr | 375 |
| Weapons_AK47_3P.adr | 375 |
| Weapons_RiotShotgun_3P.adr | 175 |
| Weapons_M40Sniper_3P.adr | 659 |
| Weapons_RanchRifle_3P.adr | 350 |
| Weapons_MK46_3P.adr (LMG) | 250 |
| Weapon_Pistol_45Auto_3P.adr (M1911) | 251 |
| Weapons_M9_3P.adr | 251 |
| Weapons_Magnum_3P.adr | 251 |
| Weapons_Crossbow01_3P.adr | 120 |
| Weapons_Bow01_3P.adr | 75 |

De øvrige leverede navne:

```text
Weapon_Empty.adr                         (fists)
Weapon_Binoculars_3P.adr
Weapons_Grenades_SmokeGrenade_3P.adr
Weapons_MolotovCocktail_3P.adr
Weapons_Grenades_FlashBang_3P.adr
Weapons_Grenades_HEGrenade_3P.adr
Weapons_Grenades_GasGrenade_3P.adr
```

Disse har ingen anvendt projektilhastighed og aktiverer ikke aim. Et ukendt
våben får heller ikke automatisk standardhastigheden 375 m/s.

## Skelet og køretøjer fra den oprindelige guide

Knogleindeks: hoved 26, hals 24, bryst 23, ryg 22, bækken 12; venstre arm
52/54/55, højre arm 76/78/79; venstre ben 13/14/16, højre ben 110/111/113.
Stride 0x30. Kæden er entity+0x5C0 -> +0x250 -> +0x50 -> +0x28.
Rotationen er x'=feet.x+x*cos(yaw)-z*sin(yaw), y'=feet.y+y,
z'=feet.z+x*sin(yaw)+z*cos(yaw).

Guiden angiver mount-pointer ved `actor+0x3A08`, men definerer ikke entydigt
actor i det afsnit. Implementationen bruger den tidligere guides skeleton-actor
fra `entity+0x5C0`. **Det er en dokumenteret antagelse, som kræver validering.**
`+0x18` beskrives som et felt i en anden struktur (`CEntityInfo`) og bruges ikke
som automatisk alternativ.

Ved et gyldigt mount bruges køretøjets position +0x9D0 og hastighed +0x430.
Køretøjs-HP vises som ukendt. Fejlede mount-læsninger markeres som ukendte;
de må ikke stiltiende tolkes som "på jorden" og bruges til aim.

Der leveres ingen implementering af en synlighedstest i kildematerialet.

## Fravalg og næste arbejde

Brugeren har bedt om at fjerne magic bullet og den faste Windows-brugerkontrol.
Driverarbejde er udskudt. Der er ikke indlæst en driver, startet en mapper eller
tilføjet kode til at omgå spillets beskyttelse.
