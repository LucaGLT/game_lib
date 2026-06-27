# Eldhôm — Carte Base, Tecniche d’Arma e Carte Comportamento Mostri

Versione: bozza consolidata  
Formato: Markdown operativo  
Uso: prototipo, playtest, riferimento per AI/regole

---

# Indice

1. [Regole comuni di riferimento](#1-regole-comuni-di-riferimento)
2. [Carte Base per Tutti i Giocatori](#2-carte-base-per-tutti-i-giocatori)
3. [Carte Tecniche d’Arma — Arco e Balestra](#3-carte-tecniche-darma--arco-e-balestra)
4. [Mazzetto Precisione di Tiro e Lancio](#4-mazzetto-precisione-di-tiro-e-lancio)
5. [Carte Comportamento dei Mostri](#5-carte-comportamento-dei-mostri)
6. [Mazzo Comportamento — Brigante Comune](#6-mazzo-comportamento--brigante-comune)
7. [Mazzo Comportamento — Brigante con Frombola](#7-mazzo-comportamento--brigante-con-frombola)

---

# 1. Regole comuni di riferimento

## 1.1 Azioni Semplici aggiornate

```text
Movimento Semplice
- Muovi fino a 2 Locazioni
- Costo: 2⌛

Attacco Semplice
- Infliggi 1❌ in mischia
- Costo: 2⌛

Interazione Semplice
- Interagisci con un elemento della scena
- Costo: 3⌛

Recupero Semplice
- Recupera 1 PV
- Puoi scartare fino a 2 carte dalla Mano e pescare altrettante carte
- Puoi ricaricare oggetti o carte ricaricabili con Recupero
- Costo: 3⌛
```

## 1.2 Tipi di Carta Azione

```text
Singola
- La carta viene giocata, risolta e scartata.
- Dopo una Carta Singola, il turno termina.

Inizio Sequenza
- Può essere giocata come prima carta del turno.
- Dopo la risoluzione, il PG può fermarsi oppure continuare con:
  - una Carta Continuo Sequenza;
  - una Carta Fine Sequenza.

Continuo Sequenza
- Può essere giocata solo dopo:
  - una Carta Inizio Sequenza;
  - oppure un’altra Carta Continuo Sequenza.
- Dopo la risoluzione, il PG può fermarsi oppure continuare con:
  - una Carta Continuo Sequenza;
  - una Carta Fine Sequenza.

Fine Sequenza
- Può essere giocata solo dentro una Sequenza.
- Dopo la risoluzione, il turno termina.

Istantanea
- Può essere giocata fuori dal proprio turno se il Trigger è soddisfatto.
- Fa avanzare il cubetto del PG sull’Asse Temporale del costo indicato.
```

## 1.3 Icone

```text
▶️ Movimento / Posizionamento
⏸️ Attacco / Danno / Pressione offensiva
⏺️ Interazione / Supporto / Preparazione
⚡ Istantanea / Reazione
```

Le icone indicano la natura dell’effetto.  
Non indicano quale Azione Semplice viene sostituita.

## 1.4 Regola generale — Prima Linea e Retroguardia

Ogni Locazione può contenere miniature in:

```text
Prima Linea
Retroguardia
```

Per ogni fazione presente nella Locazione:

```text
Retroguardia ≤ Prima Linea
```

Questa regola si applica separatamente a:

```text
PG e Alleati dei PG
Mostri e Alleati dei Mostri
altre eventuali fazioni
```

## 1.5 Regola generale — Bersagli

```text
Solo la Prima Linea può essere bersagliata nella regola base.
La Retroguardia può essere bersagliata solo se una Carta, Arma, Scheda Mostro o effetto lo permette esplicitamente.
```

## 1.6 Attacchi in mischia

```text
La Mischia avviene nella stessa Locazione.

Solo una miniatura in Prima Linea può attaccare in Mischia.
Solo una miniatura nemica in Prima Linea può essere bersagliata in Mischia.

Regola base:
PL → PL
```

## 1.7 Attacchi a distanza

```text
Un Attacco a Distanza può essere eseguito normalmente solo da una miniatura in Retroguardia.
```

Eccezioni:

```text
- una Carta dice esplicitamente che può essere usata anche in Prima Linea;
- un’Arma dice esplicitamente che può essere usata anche in Prima Linea;
- una Scheda Mostro dice esplicitamente che il Mostro può attaccare a distanza anche in Prima Linea.
```

La Prima Linea può attaccare a Distanza > 0 solo se non ci sono Nemici nella sua stessa Locazione.

## 1.8 Distanze

```text
Mischia = stessa Locazione, attacco corpo a corpo
Distanza corta = stessa Locazione, attacco a distanza interno
Distanza breve = Locazione adiacente
Distanza media = 2 Locazioni
Distanza lunga = 3+ Locazioni, solo se indicato
```

---

# 2. Carte Base per Tutti i Giocatori

Ogni PG inizia con queste 15 Carte Base.

```text
Origine: Base
Disponibilità: tutti i PG
Numero copie: 1 copia ciascuna
Funzione: mazzo iniziale universale
Potenza: bassa
Identità: nessuna Etnia, nessuna Classe, nessuna Affiliazione
```

---

## 1. Passo Cauto

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ▶️  
**Costo:** 2⌛

**Effetto:**  
Muovi fino a 2 Locazioni.

Durante questo Movimento ignora il primo effetto di:

```text
Terreno Difficile
Passaggio Stretto
Fumo
Buio
```

Non puoi attraversare Locazioni occupate da Nemici.

Quando entri nella Locazione finale, si risolve normalmente lo Schieramento.

**Ruolo:** Movimento Semplice più sicuro.

---

## 2. Scatto Breve

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ▶️  
**Costo:** 2⌛

**Effetto:**  
Muovi fino a 3 Locazioni.

Non puoi attraversare Locazioni occupate da Nemici.

Puoi entrare in una Locazione occupata da Nemici solo come Locazione finale.

Quando entri nella Locazione finale, si risolve normalmente lo Schieramento.

**Ruolo:** Movimento Semplice più lungo.

---

## 3. Assestarsi

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ▶️⏺️  
**Costo:** 1⌛

**Effetto:**  
Opera uno Schieramento dei PG nella tua Locazione.

Limitazione:

```text
Se nella tua Locazione sono presenti Nemici,
puoi spostare solo te stesso.
```

La Formazione finale deve rispettare sempre:

```text
Retroguardia ≤ Prima Linea
```

**Ruolo:** gestione minima della Formazione.

---

## 4. Colpo Secco

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏸️  
**Costo:** 2⌛

**Effetto:**  
Attacca un Nemico in mischia nella tua Locazione.

Infliggi 1❌.

Se tu e il bersaglio siete entrambi in Prima Linea, scegli uno:

```text
- pesca 1 carta, poi scarta 1 carta;
- oppure il bersaglio non può passare volontariamente in Retroguardia
  fino alla sua prossima attivazione.
```

**Ruolo:** Attacco Semplice leggermente migliorato.

---

## 5. Fendente Pesante

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏸️  
**Costo:** 3⌛

**Requisito:**  
Devi essere in Prima Linea.

**Effetto:**  
Attacca un Nemico in mischia nella tua Locazione.

Infliggi 2❌.

**Ruolo:** colpo base più forte, ma lento.

---

## 6. Spinta di Corpo

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏸️▶️  
**Costo:** 3⌛

**Requisito:**  
Devi essere in Prima Linea.

**Effetto:**  
Attacca un Nemico in mischia nella tua Locazione.

Infliggi 1❌.

Se il bersaglio è in Prima Linea, puoi Spingerlo in Retroguardia.

Se questo rende illegale la Formazione dei Mostri, si risolve uno Scompaginamento dei Mostri.

Tu sei il Giocatore che ha causato lo Scompaginamento.

**Ruolo:** poco danno, ma può rompere la posizione nemica.

---

## 7. Colpo d’Apertura

**Origine:** Base  
**Tipo:** Inizio Sequenza  
**Icone:** ⏸️  
**Costo:** 2⌛

**Effetto:**  
Attacca un Nemico in mischia nella tua Locazione.

Infliggi 1❌.

Puoi continuare la Sequenza.

**Ruolo:** apre una Sequenza offensiva.

---

## 8. Passo e Lama

**Origine:** Base  
**Tipo:** Inizio Sequenza  
**Icone:** ▶️⏸️  
**Costo:** 3⌛

**Effetto:**  
Muovi di 1 Locazione.

Quando entri nella nuova Locazione, si risolve normalmente lo Schieramento.

Poi puoi attaccare un Nemico in mischia nella tua Locazione.

Infliggi 1❌.

Puoi continuare la Sequenza.

**Ruolo:** Movimento + Attacco, ma costoso.

---

## 9. Secondo Colpo

**Origine:** Base  
**Tipo:** Continuo Sequenza  
**Icone:** ⏸️  
**Costo:** 2⌛

**Condizione:**  
Puoi giocarla solo dopo una Carta Inizio Sequenza o una Carta Continuo Sequenza.

**Effetto:**  
Attacca un Nemico in mischia nella tua Locazione.

Infliggi 1❌.

Se il bersaglio è lo stesso bersaglio colpito dalla carta precedente della Sequenza, infliggi +1❌.

Puoi continuare la Sequenza.

**Ruolo:** carta di combo. Forte solo se resti sullo stesso bersaglio.

---

## 10. Pressione Continua

**Origine:** Base  
**Tipo:** Continuo Sequenza  
**Icone:** ⏸️▶️  
**Costo:** 2⌛

**Condizione:**  
Puoi giocarla solo dopo una Carta Inizio Sequenza o una Carta Continuo Sequenza.

**Effetto:**  
Scegli uno:

```text
A. Attacca un Nemico in mischia nella tua Locazione.
   Infliggi 1❌.

B. Opera uno Schieramento dei PG nella tua Locazione.
   Se nella Locazione sono presenti Nemici, puoi spostare solo te stesso.
```

Puoi continuare la Sequenza.

**Ruolo:** mantiene pressione oppure corregge posizione durante una Sequenza.

---

## 11. Colpo di Chiusura

**Origine:** Base  
**Tipo:** Fine Sequenza  
**Icone:** ⏸️  
**Costo:** 2⌛

**Condizione:**  
Puoi giocarla solo dentro una Sequenza.

**Effetto:**  
Attacca un Nemico in mischia nella tua Locazione.

Infliggi 1❌.

Se durante questa Sequenza hai già inflitto almeno 2❌ allo stesso bersaglio, infliggi invece 2❌.

Dopo questa carta, la Sequenza termina.

**Ruolo:** chiusura offensiva. Forte solo se la Sequenza è già riuscita.

---

## 12. Lancio di Fortuna

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏸️  
**Costo:** 2⌛

**Effetto:**  
Attacca un Nemico a distanza breve.

Puoi giocare questa carta anche senza arma a distanza.

Rappresenta il lancio di:

```text
pietra
coccio
pezzo di ferro
utensile
frammento trovato sul posto
```

**Danno:**  
Infliggi 1❌.

**Limitazione:**  
Non puoi bersagliare un Nemico in Retroguardia se nella sua Locazione ci sono Nemici in Prima Linea.

**Ruolo:** attacco a distanza improvvisato, debole, universale.

---

## 13. Uso Arco/Balestra

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏸️  
**Costo:** 2⌛

**Requisito:**  
Devi avere equipaggiato un Arco o una Balestra.

**Posizione:**  
Devi essere in Retroguardia, salvo effetto specifico.

**Effetto:**  
Attacca un Nemico a distanza breve o media, secondo la portata dell’arma equipaggiata.

Se si usa il mazzetto Precisione, questa carta effettua un Tiro.

```text
Danno finale = Danno arma + Modificatore Precisione
```

Se non si usa il mazzetto Precisione, infliggi il danno indicato dall’arma.

Se l’arma non indica danno, infliggi 1❌.

Se non hai un Arco o una Balestra equipaggiati, questa carta non può essere giocata.

**Ruolo:** carta base per usare armi da tiro.

---

## 14. Mano Ferma

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏺️  
**Costo:** 2⌛

**Effetto:**  
Esegui una Interazione nella tua Locazione.

Questa Interazione può riguardare solo elementi semplici:

```text
leva
porta
forziere
oggetto a terra
meccanismo evidente
ostacolo leggero
elemento di scenario non complesso
```

Non può essere usata per:

```text
rituali
enigmi complessi
macchinari avanzati
disinnesco complesso
interazioni che richiedono più fasi
obiettivi principali della Missione, salvo indicazione contraria
```

**Ruolo:** Interazione Semplice più rapida, ma limitata.

---

## 15. Riprendere Fiato

**Origine:** Base  
**Tipo:** Singola  
**Icone:** ⏺️  
**Costo:** 3⌛

**Effetto:**  
Recupera 1 PV.

Poi puoi scartare 1 carta dalla Mano e pescare 1 carta.

Se sei in Retroguardia, puoi invece scartare fino a 2 carte dalla Mano e pescarne altrettante.

Non puoi superare il tuo limite di Mano.

**Ruolo:** Recupero Semplice leggermente migliorato se sei in Retroguardia.

---

# 3. Carte Tecniche d’Arma — Arco e Balestra

Queste carte sono **Carte Base — Tecniche d’Arma**.

```text
Sono accessibili a tutti.
Non fanno parte obbligatoria delle 15 Carte Base universali.
Ha senso inserirle nel Mazzo Missione solo se il PG porta l’arma richiesta.
```

## 3.1 Regola comune — Tiro

Parola chiave:

```text
Tiro:
- Pesca 1 carta Precisione di Tiro e Lancio.
- Aggiungi il modificatore al Danno base dell’arma.
- Danno finale = Danno arma + Modificatore Precisione + eventuale Bonus Carta.
- Se il danno finale è minore di 0, diventa 0.
- Se il danno finale è 0, il bersaglio non subisce danno.
```

Danni base consigliati:

```text
Arco = 1❌
Balestra = 2❌
```

## 3.2 XP Tecnica d’Arma

Quando giochi una Carta Tecnica d’Arma e risolvi almeno un attacco contro un bersaglio valido:

```text
+1 XP nella Tecnica dell’Arma usata
+1 XP Precisione di Tiro e Lancio, se la carta ha usato il mazzetto Precisione
```

Limite consigliato:

```text
Massimo XP per Missione per singola Tecnica: 5
Massimo XP Precisione per Missione: 5
```

---

## Carte solo Arco

---

## 1. Tiro Rapido

**Origine:** Base — Arco  
**Tipo:** Inizio Sequenza  
**Icone:** ⏸️  
**Costo:** 2⌛

**Requisito:**  
Arco equipaggiato.

**Posizione:**  
Retroguardia.

**Effetto:**  
Attacca un Nemico valido entro la portata dell’Arco.

Effettua un Tiro con Arco.

```text
Danno finale = Danno Arco + Modificatore Precisione
```

Puoi continuare la Sequenza.

**XP:**  

```text
+1 XP Arco
+1 XP Precisione
```

---

## 2. Doppia Incoccata

**Origine:** Base — Arco  
**Tipo:** Continuo Sequenza  
**Icone:** ⏸️  
**Costo:** 2⌛

**Requisito:**  
Arco equipaggiato.

**Condizione:**  
Puoi giocarla solo dopo una Carta Inizio Sequenza.

**Posizione:**  
Retroguardia.

**Effetto:**  
Scegli uno:

```text
A. Attacca lo stesso bersaglio della carta precedente.
B. Attacca un secondo bersaglio valido.
```

Effettua un Tiro con Arco.

```text
Danno finale = Danno Arco + Modificatore Precisione
```

Puoi continuare la Sequenza.

**Limitazione:**  
Se scegli un secondo bersaglio, deve essere un bersaglio valido in Prima Linea.

**XP:**  

```text
+1 XP Arco
+1 XP Precisione
```

---

## Carte solo Balestra

---

## 3. Quadrello Pesante

**Origine:** Base — Balestra  
**Tipo:** Singola  
**Icone:** ⏸️  
**Costo:** 3⌛

**Requisito:**  
Balestra equipaggiata.

**Posizione:**  
Retroguardia.

**Effetto:**  
Attacca un Nemico valido entro la portata della Balestra.

Effettua un Tiro con Balestra.

```text
Danno finale = Danno Balestra + Modificatore Precisione
```

Se hai giocato **Mirare** prima di questa carta, aggiungi anche il bonus di Mirare.

**XP:**  

```text
+1 XP Balestra
+1 XP Precisione
```

---

## 4. Tiro di Arresto

**Origine:** Base — Balestra  
**Tipo:** Istantanea  
**Icone:** ⚡⏸️  
**Costo:** 2⌛

**Requisito:**  
Balestra equipaggiata.

**Trigger:**  
Un Nemico entra nella tua Locazione o in una Locazione adiacente.

**Posizione:**  
Retroguardia.

**Effetto:**  
Attacca quel Nemico con la Balestra.

Effettua un Tiro con Balestra.

```text
Danno finale = Danno Balestra + Modificatore Precisione
```

Se il bersaglio subisce almeno 1❌, scegli uno:

```text
- riduce il Movimento residuo di 1 Locazione;
- oppure entra obbligatoriamente in Prima Linea nella Locazione finale.
```

**XP:**  

```text
+1 XP Balestra
+1 XP Precisione
```

---

## Carte Arco o Balestra

---

## 5. Mirare

**Origine:** Base — Arco/Balestra  
**Tipo:** Singola  
**Icone:** ⏺️⏸️  
**Costo:** 2⌛

**Requisito:**  
Arco o Balestra equipaggiati.

**Posizione:**  
Retroguardia.

**Effetto:**  
Fino alla tua prossima attivazione, la prossima Carta Attacco a Distanza con Arco o Balestra ottiene uno:

```text
- +1❌ dopo aver pescato Precisione;
- oppure ignora Copertura Leggera.
```

L’effetto termina se:

```text
- lasci la Retroguardia;
- subisci danno;
- cambi Locazione;
- giochi una Carta diversa da Attacco a Distanza.
```

**XP:**  
Questa carta non dà XP Precisione da sola.  
Dà XP solo quando viene poi risolto l’attacco preparato.

---

## 6. Tiro di Copertura

**Origine:** Base — Arco/Balestra  
**Tipo:** Istantanea  
**Icone:** ⚡⏸️  
**Costo:** 1⌛

**Requisito:**  
Arco o Balestra equipaggiati.

**Trigger:**  
Un Alleato nella tua Locazione o in una Locazione adiacente viene attaccato da un Nemico in Prima Linea.

**Posizione:**  
Retroguardia.

**Effetto:**  
Scegli il Nemico attaccante.

Pesca 1 carta Precisione.

Se il risultato è:

```text
-1:
  l’attacco nemico non viene modificato.

0:
  l’attacco nemico infligge -1❌.

+1 o +2:
  l’attacco nemico infligge -1❌
  e non può causare Scompaginamento.
```

Se usi una Balestra e peschi `+1` o `+2`, puoi invece infliggere 1❌ al Nemico attaccante.

**XP:**  

```text
+1 XP dell’arma usata
+1 XP Precisione
```

---

## 7. Colpo sulla Linea

**Origine:** Base — Arco/Balestra  
**Tipo:** Fine Sequenza  
**Icone:** ⏸️  
**Costo:** 2⌛

**Requisito:**  
Arco o Balestra equipaggiati.

**Condizione:**  
Puoi giocarla solo dentro una Sequenza.

**Posizione:**  
Retroguardia.

**Effetto:**  
Attacca un Nemico in Prima Linea entro la portata dell’arma.

Effettua un Tiro con l’arma usata.

```text
Danno finale = Danno arma + Modificatore Precisione
```

Se durante questa Sequenza il bersaglio ha già subito danno da una tua Carta Attacco a Distanza, scegli uno:

```text
- infliggi +1❌;
- oppure il bersaglio non può passare volontariamente in Retroguardia
  fino alla sua prossima attivazione.
```

Dopo questa carta, la Sequenza termina.

**XP:**  

```text
+1 XP dell’arma usata
+1 XP Precisione
```

---

# 4. Mazzetto Precisione di Tiro e Lancio

Questo mazzetto è personale del PG.

Si usa per:

```text
Arco
Balestra
Coltelli da Lancio
Armi da Lancio
Frombola se usata da PG
altre armi a distanza leggere, se indicate
```

Non si usa per:

```text
magie
raggi
poteri eonici
effetti automatici
attacchi senza traiettoria fisica
```

## 4.1 Mazzetto base

```text
Precisione di Tiro e Lancio — Base

-1
-1
 0
 0
 0
+1
+1
```

Totale: 7 carte.

Media: 0.

## 4.2 Risoluzione

Quando una Carta richiede un Attacco a Distanza con arma compatibile:

```text
1. Controlla bersaglio valido.
2. Controlla posizione valida.
3. Pesca 1 carta Precisione.
4. Calcola il danno:
   Danno finale = Danno arma + Modificatore Precisione + Bonus carta.
5. Se il danno finale è minore di 0, diventa 0.
6. Se il danno finale è 0, il bersaglio non subisce danno.
7. Scarta la carta Precisione pescata.
8. Se il mazzetto Precisione è vuoto, rimescola gli scarti Precisione.
```

## 4.3 Evoluzione del mazzetto Precisione

### Rango 0 — Base

```text
-1
-1
 0
 0
 0
+1
+1
```

### Rango 1 — 10 XP Precisione

Rimuovi una carta `-1`.

```text
-1
 0
 0
 0
+1
+1
```

### Rango 2 — 25 XP Precisione

Aggiungi una carta `+1`.

```text
-1
 0
 0
 0
+1
+1
+1
```

### Rango 3 — 50 XP Precisione

Sostituisci una carta `0` con una carta `+1`.

```text
-1
 0
 0
+1
+1
+1
+1
```

### Rango 4 — 70 XP Precisione

Rimuovi l’ultima carta `-1`.

```text
0
0
+1
+1
+1
+1
```

### Rango 5 — 100 XP Precisione

Aggiungi una carta `+2`.

```text
0
0
+1
+1
+1
+1
+2
```

---

# 5. Carte Comportamento dei Mostri

## 5.1 Principio generale

Una Carta Comportamento Mostro non è una Carta Missione del PG.

```text
Carta Missione PG:
- è una singola azione o un pezzo di Sequenza;
- il PG costruisce la Sequenza usando più carte dalla Mano.

Carta Comportamento Mostro:
- è già una Sequenza completa;
- il Mostro non costruisce Sequenze carta per carta;
- la Carta contiene tutti i passaggi da eseguire.
```

## 5.2 Strutture possibili

```text
Azione Singola
- 1 solo effetto.
- Costo unico.
- Fine attivazione.

Sequenza 2
- Passaggio 1 — Inizio
- Passaggio 2 — Fine

Sequenza 3
- Passaggio 1 — Inizio
- Passaggio 2 — Continuo
- Passaggio 3 — Fine

Reazione
- Si attiva fuori dal turno del Gruppo quando il Trigger è soddisfatto.

Passiva + Reazione
- La Passiva resta attiva finché la Carta è attiva.
- La Reazione può consumare e sostituire la Carta.
```

## 5.3 Risoluzione di una Carta Comportamento

Quando un Gruppo di Mostri si attiva:

```text
1. Risolve la propria Carta Comportamento attiva dall’alto verso il basso.
2. Per ogni passaggio:
   - controlla quali Mostri del Gruppo possono eseguire il passaggio;
   - i Mostri validi eseguono il passaggio;
   - il segnalino del Gruppo avanza di X⌛;
   - se il passaggio permette di continuare, passa al passaggio successivo;
   - se il passaggio dice Fine, l’attivazione termina.
```

Il costo si paga una sola volta per il Gruppo, non una volta per ogni Mostro.

## 5.4 Passaggi impossibili

```text
Se almeno un Mostro del Gruppo può eseguire il passaggio:
- il passaggio viene risolto.

Se nessun Mostro può eseguire il passaggio:
- usa il Fallback indicato.

Se non c’è Fallback:
- salta quel passaggio senza pagare il costo.

Se era un passaggio Fine:
- la Carta termina.
```

## 5.5 Reazioni dei Mostri

Quando un Gruppo usa una Reazione della propria Carta Comportamento attiva:

```text
1. Risolve la Reazione.
2. Avanza il segnalino del Gruppo del costo indicato.
3. Scarta la Carta Comportamento attiva.
4. Pesca subito una nuova Carta Comportamento.
5. La nuova Carta diventa attiva per il Gruppo.
```

La Reazione interrompe una Sequenza dei PG solo se il testo lo dice esplicitamente.

---

# 6. Mazzo Comportamento — Brigante Comune

## 6.1 Scheda sintetica

```text
Nome: Brigante Comune
Categoria: Umano / Brigante
Ruolo: Mischia leggera
PV: 2
Danno base: 1❌
Movimento: 2 Locazioni
Armi: Bastone / Pugnale
Portata: Mischia
Schieramento: Prima Linea, salvo Carta diversa
```

## 6.2 Identità tattica

```text
- va in Prima Linea;
- combatte in mischia;
- usa colpi sporchi;
- può Spingere;
- può causare Scompaginamento;
- è pericoloso in gruppo.
```

## 6.3 Schieramento naturale

```text
Questo Mostro si Schiera in Prima Linea, salvo che una Carta dica diversamente.
```

---

## Carta 1 — Colpo di Bastone

**Struttura:** Azione Singola  
**Costo:** 2⌛

**Condizione:**  
Almeno un Brigante Comune del Gruppo ha un PG in mischia.

**Bersaglio:**  
PG in Prima Linea nella stessa Locazione.

**Effetto:**  
Ogni Brigante Comune del Gruppo che ha un bersaglio valido attacca in mischia.

Infligge 1❌.

**Fallback:**  
Se nessun Brigante può attaccare, ogni Brigante muove di 1 Locazione verso il PG più vicino.

Costo Fallback: 1⌛.

---

## Carta 2 — Addosso!

**Struttura:** Sequenza 2  
**Costo totale possibile:** 3⌛

### Passaggio 1 — Inizio

**Costo:** 1⌛

Ogni Brigante Comune muove di 1 Locazione verso il PG più vicino.

Se entra in una Locazione con PG, entra in Prima Linea.

Risolvi lo Schieramento dei Mostri.

### Passaggio 2 — Fine

**Costo:** 2⌛

Ogni Brigante Comune che ha un PG in mischia attacca.

Infligge 1❌.

Se nessun Brigante può attaccare, questo passaggio non si risolve.

---

## Carta 3 — Spintone Sporco

**Struttura:** Azione Singola  
**Costo:** 3⌛

**Condizione:**  
Almeno un Brigante Comune ha un PG in Prima Linea nella stessa Locazione.

**Effetto:**  
Ogni Brigante Comune in mischia attacca.

Infligge 1❌.

Poi scegli il primo PG danneggiato da questa Carta.

Quel PG viene Spinto in Retroguardia, se possibile.

Se questo rompe la Formazione dei PG, si risolve uno Scompaginamento dei PG.

Questa Carta causa lo Scompaginamento.

---

## Carta 4 — Calca Violenta

**Struttura:** Sequenza 3  
**Costo totale possibile:** 5⌛

### Passaggio 1 — Inizio

**Costo:** 1⌛

Ogni Brigante Comune muove di 1 Locazione verso la Locazione con più PG raggiungibile.

Se entra in una Locazione con PG, entra in Prima Linea.

### Passaggio 2 — Continuo

**Costo:** 2⌛

Ogni Brigante Comune in mischia attacca un PG in Prima Linea.

Infligge 1❌.

### Passaggio 3 — Fine

**Costo:** 2⌛

Se almeno un PG ha subito danno durante il Passaggio 2, quel PG non può passare volontariamente in Retroguardia fino alla sua prossima attivazione.

Se il PG era già in Retroguardia a causa di uno Scompaginamento, subisce invece +1❌.

---

## Carta 5 — Pugnalata Bassa

**Struttura:** Azione Singola  
**Costo:** 2⌛

**Condizione:**  
Almeno un Brigante Comune è in mischia con un PG.

**Effetto:**  
Ogni Brigante Comune attacca.

Infligge 1❌.

Se il bersaglio ha già subito danno da un Brigante in questa attivazione, infligge +1❌.

---

## Carta 6 — Colpo di Ritorno

**Struttura:** Reazione  
**Costo:** 1⌛

**Trigger:**  
Un PG nella stessa Locazione lascia la Prima Linea o esce dalla Locazione.

**Effetto:**  
Un solo Brigante Comune del Gruppo, nella stessa Locazione del PG, attacca quel PG in mischia.

Infligge 1❌.

Questa Reazione non interrompe la Sequenza del PG.

Dopo aver usato questa Reazione, scarta questa Carta e pesca una nuova Carta Comportamento.

---

# 7. Mazzo Comportamento — Brigante con Frombola

## 7.1 Scheda sintetica

```text
Nome: Brigante con Frombola
Categoria: Umano / Brigante
Ruolo: Tiratore leggero
PV: 2
Danno pugnale: 1❌
Danno frombola: 1❌
Portata frombola: 2 Locazioni
Movimento: 2 Locazioni
Armi: Frombola / Pugnale
Schieramento: Retroguardia se la Formazione resta valida, altrimenti Prima Linea
```

## 7.2 Identità tattica

```text
- predilige Retroguardia;
- attacca a distanza;
- usa Frombola entro 2 Locazioni;
- evita la mischia;
- se finisce in Prima Linea usa il Pugnale;
- sfrutta Briganti Comuni come copertura.
```

## 7.3 Regola Frombola

```text
La Frombola è un Attacco a Distanza.

Può essere usata solo se il Brigante con Frombola è in Retroguardia.

Portata massima: 2 Locazioni.

Non può bersagliare un PG in Retroguardia
se nella Locazione del bersaglio ci sono PG in Prima Linea.
```

## 7.4 Schieramento naturale

```text
Questo Mostro si Schiera in Retroguardia se la Formazione resta valida.
Altrimenti si Schiera in Prima Linea.
```

Se nella stessa Locazione ci sono Briganti Comuni:

```text
I Briganti Comuni vanno in Prima Linea prima dei Briganti con Frombola.
I Briganti con Frombola vanno in Retroguardia finché la Formazione resta valida.
```

---

## Carta 1 — Sassata di Frombola

**Struttura:** Azione Singola  
**Costo:** 2⌛

**Condizione:**  
Almeno un Brigante con Frombola del Gruppo è in Retroguardia e ha un PG valido entro 2 Locazioni.

**Effetto:**  
Ogni Brigante con Frombola in Retroguardia effettua un Attacco a Distanza con Frombola.

Infligge 1❌.

**Bersaglio:**  
PG valido più vicino.

In caso di parità:

```text
1. PG con meno PV
2. PG più indietro sulla Linea Temporale
3. scelgono i Giocatori
```

**Fallback:**  
Se nessun Brigante può tirare, risolvi il Movimento di **Cercare Posizione**.

---

## Carta 2 — Cercare Posizione

**Struttura:** Sequenza 2  
**Costo totale possibile:** 4⌛

### Passaggio 1 — Inizio

**Costo:** 2⌛

Ogni Brigante con Frombola muove fino a 2 Locazioni verso una Locazione da cui almeno un PG sia entro 2 Locazioni.

Quando entra nella Locazione finale, si Schiera in Retroguardia se la Formazione resta valida.

Altrimenti si Schiera in Prima Linea.

### Passaggio 2 — Fine

**Costo:** 2⌛

Ogni Brigante con Frombola in Retroguardia che ha un bersaglio valido entro 2 Locazioni effettua una Sassata di Frombola.

Infligge 1❌.

Se nessun Brigante può tirare, questo passaggio non si risolve.

---

## Carta 3 — Tiro sul Ferito

**Struttura:** Azione Singola  
**Costo:** 2⌛

**Condizione:**  
Almeno un Brigante con Frombola è in Retroguardia.

**Effetto:**  
Ogni Brigante con Frombola in Retroguardia attacca a distanza un PG valido entro 2 Locazioni.

Bersaglia il PG valido con meno PV.

Infligge 1❌.

Se il bersaglio ha 2 PV o meno, infligge +1❌.

---

## Carta 4 — Indietro e Tira

**Struttura:** Sequenza 2  
**Costo totale possibile:** 4⌛

### Passaggio 1 — Inizio

**Costo:** 2⌛

Ogni Brigante con Frombola in Prima Linea tenta di passare in Retroguardia tramite Schieramento dei Mostri.

Se la Formazione non lo consente, resta in Prima Linea.

### Passaggio 2 — Fine

**Costo:** 2⌛

Ogni Brigante con Frombola in Retroguardia effettua una Sassata di Frombola contro un PG valido entro 2 Locazioni.

Infligge 1❌.

Se nessun Brigante può tirare, questo passaggio non si risolve.

---

## Carta 5 — Sassaiola

**Struttura:** Azione Singola  
**Costo:** 3⌛

**Condizione:**  
Almeno 2 Briganti con Frombola del Gruppo sono in Retroguardia e hanno linea di tiro verso la stessa Locazione entro 2 Locazioni.

**Effetto:**  
Scegli la Locazione valida con più PG.

Ogni PG in Prima Linea in quella Locazione subisce 1❌.

I PG in Retroguardia non vengono colpiti se nella loro Locazione ci sono PG in Prima Linea.

**Fallback:**  
Se la condizione non è soddisfatta, risolvi **Sassata di Frombola**.

---

## Carta 6 — Coltello d’Emergenza

**Struttura:** Sequenza 2  
**Costo totale possibile:** 4⌛

### Passaggio 1 — Inizio

**Costo:** 2⌛

Ogni Brigante con Frombola in Prima Linea nella stessa Locazione di un PG attacca con il Pugnale.

Infligge 1❌.

### Passaggio 2 — Fine

**Costo:** 2⌛

Dopo l’attacco, ogni Brigante con Frombola tenta di passare in Retroguardia tramite Schieramento dei Mostri.

Se la Formazione non lo consente, resta in Prima Linea.

---

## Carta 7 — Sasso d’Interdizione

**Struttura:** Reazione  
**Costo:** 1⌛

**Trigger:**  
Un PG entro 2 Locazioni inizia un Movimento che lo porterebbe più vicino a un Brigante con Frombola del Gruppo.

**Condizione:**  
Almeno un Brigante con Frombola del Gruppo è in Retroguardia e ha un bersaglio valido.

**Effetto:**  
Un solo Brigante con Frombola effettua una Sassata di Frombola contro quel PG.

Infligge 1❌.

Poi scegli uno:

```text
- il PG riduce il Movimento di 1 Locazione;
- oppure il PG completa il Movimento ma deve entrare in Prima Linea
  nella Locazione finale, se entra in una Locazione con Nemici.
```

Questa Reazione non interrompe la Sequenza del PG.

Dopo aver usato questa Reazione, scarta questa Carta e pesca una nuova Carta Comportamento.

---

# Appendice — Sintesi mazzi

## Carte Base universali

```text
1. Passo Cauto
2. Scatto Breve
3. Assestarsi
4. Colpo Secco
5. Fendente Pesante
6. Spinta di Corpo
7. Colpo d’Apertura
8. Passo e Lama
9. Secondo Colpo
10. Pressione Continua
11. Colpo di Chiusura
12. Lancio di Fortuna
13. Uso Arco/Balestra
14. Mano Ferma
15. Riprendere Fiato
```

## Carte Tecnica Arco/Balestra

```text
1. Tiro Rapido — Arco
2. Doppia Incoccata — Arco
3. Quadrello Pesante — Balestra
4. Tiro di Arresto — Balestra
5. Mirare — Arco/Balestra
6. Tiro di Copertura — Arco/Balestra
7. Colpo sulla Linea — Arco/Balestra
```

## Brigante Comune

```text
1. Colpo di Bastone
2. Addosso!
3. Spintone Sporco
4. Calca Violenta
5. Pugnalata Bassa
6. Colpo di Ritorno
```

## Brigante con Frombola

```text
1. Sassata di Frombola
2. Cercare Posizione
3. Tiro sul Ferito
4. Indietro e Tira
5. Sassaiola
6. Coltello d’Emergenza
7. Sasso d’Interdizione
```
