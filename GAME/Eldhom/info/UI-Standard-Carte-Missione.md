# Specifica UI Standard — Carte Missione

Versione: 0.1  
Ambito: carte di tipo **Carta Missione** per il simulatore di *Le Pergamene di Eldhôm*.

---

## 1. Obiettivo della UI

La UI di una Carta Missione deve permettere al giocatore di capire rapidamente:

1. che tipo di carta è;
2. quando può essere giocata;
3. quanto costa in Tempo;
4. quale Azione o effetto produce;
5. quali condizioni, trigger o limiti applica.

Le informazioni tecniche utili al simulatore devono restare accessibili, ma non devono disturbare la lettura della carta durante il gioco.

---

## 2. Principio generale

Ogni Carta Missione deve avere due livelli di visualizzazione:

### 2.1 Vista Compatta

Usata nella mano del giocatore, negli scarti, nel mazzo o in liste rapide.

Deve mostrare solo:

- icona principale della carta;
- nome;
- costo in ⌛;
- tipo di uso;
- eventuale badge di stato essenziale, per esempio Consumabile.

Esempio:

```text
▶️ Passo Sicuro          ⌛3   📄
⏸️ Colpo Deciso          ⌛3   📄
⏺️ Mano Sicura           ⌛2   📄
⚡ Parata Istintiva      ⌛1   ⚡
🧪 Pozione di Cura       ⌛1   🧪
```

### 2.2 Vista Dettaglio

Usata quando il giocatore seleziona o ispeziona una carta.

Deve mostrare:

- intestazione completa;
- tipo carta;
- categoria di uso;
- origine;
- costo;
- effetto rapido a icone;
- testo completo;
- trigger e condizioni;
- note di gioco;
- dettagli tecnici, solo in sezione collassabile.

---

## 3. Informazioni sempre visibili

Questi campi devono essere sempre visibili in ogni Carta Missione, quando esistono.

| Campo | Obbligatorio | Descrizione |
|---|---:|---|
| Nome | Sì | Nome leggibile della carta. |
| Icona principale | Sì | Icona dell’azione o funzione dominante. |
| Costo ⌛ | Sì | Costo in Clessidre. |
| Tipo uso | Sì | Singola, Istantanea, Inizio Sequenza, Continuo, Fine, Oggetto, Preparazione. |
| Origine | Sì | Base, Missione, Affiliazione, Etnia, Equipaggiamento ecc. |
| Effetto rapido | Sì | Sintesi iconica dell’effetto. |
| Testo completo | Sì | Testo breve ma non ambiguo. |
| Trigger | Solo se presente | Quando la carta può essere giocata. |
| Condizioni | Solo se presenti | Vincoli di posizione, bersaglio, sequenza, ecc. |
| Note | Solo se necessarie | Chiarimenti non centrali. |

---

## 5. Icone standard

### 5.1 Azioni principali

| Icona | Significato |
|---|---|
| ▶️ | Movimento |
| ⏸️ | Attacco |
| ⏺️ | Interazione |
| ⚡ | Istantanea / Reazione |
| 🧪 | Oggetto / Consumabile |
| 🧠 | Preparazione / Memoria |
| 🛡️ | Difesa / riduzione Danno |
| ❤️ | Cura / recupero PV |
| 🤝 | Supporto / Alleato |

### 5.2 Tipo di uso

| Icona | Tipo |
|---|---|
| 📄 | Carta Singola |
| ⚡ | Carta Istantanea |
| 🟢 | Inizio Sequenza |
| 🟡 | Continuo Sequenza |
| 🔴 | Fine Sequenza |
| 🧠 | Preparazione / Memoria |
| 🧪 | Oggetto |

### 5.3 Risorse ed effetti

| Icona | Significato |
|---|---|
| ⌛ | Tempo / Clessidre |
| ❌ | Danno |
| ◻️ | Locazione |
| PV | Punti Vita |
| 🎯 | Bersaglio |
| 👤 | PG / Personaggio |
| 👹 | Mostro / Nemico |
| 📍 | Stessa Locazione |
| ↔️ | Collegamento / adiacenza |
| 🔁 | Continua / ripeti |
| ⛔ | Non può / vietato |
| ✅ | Può / valido |

### 5.4 Formazione

| Icona | Significato |
|---|---|
| 🧱 | Prima Linea |
| 🏹 | Retroguardia |

---

## 6. Struttura della Vista Dettaglio

Ogni Carta Missione in Vista Dettaglio deve seguire questo ordine.

```text
[ICONA PRINCIPALE] Nome Carta                         ⌛ X

[TIPO USO]   [ORIGINE]   [EVENTUALI BADGE]

EFFETTO RAPIDO
[icone sintetiche]

TESTO
[testo completo della carta]

TRIGGER
[solo se presente]

CONDIZIONI
[solo se presenti]

NOTE
[solo se necessarie]


```

---

## 7. Regole di scrittura del testo carta

### 7.1 Testo breve

Il testo deve essere breve, diretto, leggibile durante il gioco.

Usare frasi come:

```text
Muovi fino a 3 Locazioni.
Infliggi 2❌.
Effettua 1 Interazione nella tua Locazione.
Riduci di 1❌ il Danno ricevuto.
```

Evitare testi lunghi nella carta. Le regole complete devono stare nel regolamento o in tooltip estesi.

### 7.2 Numeri e simboli

Usare sempre:

- `⌛ 3` per il costo;
- `2❌` per il Danno;
- `+3 PV` per la Cura;
- `3◻️` per il Movimento in Locazioni.

### 7.3 Trigger

I trigger devono iniziare sempre con “Quando”.

Esempi:

```text
Quando subisci Danno da un Attacco.
Quando un Alleato nella tua Locazione subisce un Attacco.
Quando un Mostro termina un Movimento nella tua Locazione.
```

### 7.4 Condizioni

Le condizioni devono essere in una sezione separata, non nascoste nel testo principale.

Esempi:

```text
Condizione: solo in Mischia.
Condizione: solo durante una Sequenza.
Condizione: richiede un Alleato nella tua Locazione.
```

---

## 8. Badge standard

I badge sono etichette compatte che aiutano a leggere la carta.

### 8.1 Origine

| Badge | Significato |
|---|---|
| ⚪ Base | Carta base o comune |
| 📜 Missione | Carta concessa dalla Missione |
| 🦁 Leone | Compagnia del Leone |
| 🃏 Fato | Cartomanti del Fato |
| 🧪 Atanor | Corporazioni dell’Atanor Alchemico |
| 🌿 Etnia | Carta legata all’Etnia |
| 🏹 Equip | Carta Equipaggiamento |

### 8.2 Stato speciale

| Badge | Significato |
|---|---|
| ♻️ Scartabile | Va negli Scarti dopo l’uso |
| 🗑️ Elimina | Eliminata dalla Missione dopo l’uso |
| 🔒 Vincolata | Usabile solo con condizione specifica |
| 🧠 Preparata | Rimane attiva fino al prossimo uso valido |
| ⛓️ Sequenza | Fa parte di una Sequenza |

---

## 9. Colori consigliati

I colori devono rinforzare le icone, non sostituirle.

| Categoria | Colore suggerito |
|---|---|
| Movimento ▶️ | Blu |
| Attacco ⏸️ | Rosso |
| Interazione ⏺️ | Oro / giallo |
| Istantanea ⚡ | Viola |
| Oggetto 🧪 | Verde |
| Difesa 🛡️ | Grigio / acciaio |
| Cura ❤️ | Rosso chiaro / verde |
| Sequenza ⛓️ | Bordo dedicato o gradiente discreto |

La UI deve restare leggibile anche senza colore. Le emoji e il testo devono bastare.

---

## 10. Formato dati consigliato

Ogni Carta Missione dovrebbe avere almeno questi campi logici.

```json
{
  "id": "missione0_passo_sicuro",
  "name": "Passo Sicuro",
  "cardCategory": "mission",
  "useType": "single",
  "actionIcons": ["move"],
  "origin": "mission",
  "cost": 3,
  "quickEffect": "▶️ 3◻️",
  "rulesText": "Muovi fino a 3 Locazioni.",
  "trigger": null,
  "conditions": [],
  "notes": [],
  "badges": ["📄", "📜 Missione"],
  "technical": {
    "ruleGroup": "movement",
    "zone": "CardHand",
    "effectKey": "move_3_locations"
  }
}
```

Valori consigliati per `useType`:

```text
single
instant
sequence_start
sequence_continue
sequence_end
preparation
object
```

Valori consigliati per `actionIcons`:

```text
move
attack
interaction
instant
object
defense
healing
support
preparation
```

---

## 11. Esempi — Carte Missione 0 Simulazione A

### 11.1 Passo Sicuro

```text
▶️ Passo Sicuro                                      ⌛ 3

📄 Singola   📜 Missione

EFFETTO RAPIDO
▶️ 3◻️

TESTO
Muovi fino a 3 Locazioni.
```

---

### 11.2 Colpo Deciso

```text
⏸️ Colpo Deciso                                      ⌛ 3

📄 Singola   📜 Missione

EFFETTO RAPIDO
⏸️ 2❌

TESTO
Effettua un Attacco in Mischia.
Infliggi 2❌.
```

---

### 11.3 Mano Sicura

```text
⏺️ Mano Sicura                                       ⌛ 2

📄 Singola   📜 Missione

EFFETTO RAPIDO
⏺️ Interazione

TESTO
Effettua 1 Interazione nella tua Locazione.
```

---

### 11.4 Parata Istintiva

```text
⚡ Parata Istintiva                                  ⌛ 1

⚡ Istantanea   📜 Missione

EFFETTO RAPIDO
🛡️ -1❌

TRIGGER
Quando subisci Danno da un Attacco.

TESTO
Riduci di 1❌ il Danno ricevuto, fino a un minimo di 0❌.
```

---

### 11.5 Protezione Istintiva

```text
⚡ Protezione Istintiva                              ⌛ 1

⚡ Istantanea   📜 Missione

EFFETTO RAPIDO
🤝🛡️ Prendi il colpo

TRIGGER
Quando un Alleato nella tua Locazione subisce un Attacco.

TESTO
Puoi spostarti immediatamente in Prima Linea e subire tu tutti i Danni e gli Effetti di quell’Attacco al suo posto.

NOTE
Questo effetto non è Movimento.
Non permette di cambiare Locazione.
```

---

### 11.6 Pozione di Cura

```text
🧪 Pozione di Cura                                   ⌛ 1

🧪 Oggetto   📜 Missione   🗑️ Elimina

EFFETTO RAPIDO
❤️ +3 PV

TESTO
Recupera 3 PV a te stesso o a un Alleato nella tua Locazione.

NOTE
Se usata su un PG Caduto, rimuove Caduto e il PG rientra con 3 PV.
Dopo l’uso, elimina questa carta dalla Missione.
```

---

## 12. Esempi — Carte Sequenza Simulazione B

### 12.1 Apertura Rapida

```text
🟢 Apertura Rapida                                   ⌛ 1

🟢 Inizio Sequenza   📜 Missione   ⛓️ Sequenza

EFFETTO RAPIDO
▶️ 1◻️ → 🔁

TESTO
Muovi fino a 1 Locazione.
Puoi continuare la Sequenza.
```

---

### 12.2 Colpo d’Apertura

```text
🟢 Colpo d’Apertura                                  ⌛ 2

🟢 Inizio Sequenza   📜 Missione   ⛓️ Sequenza

EFFETTO RAPIDO
⏸️ 1❌ → 🔁

TESTO
Effettua un Attacco in Mischia.
Infliggi 1❌.
Puoi continuare la Sequenza.
```

---

### 12.3 Passo Laterale

```text
🟡 Passo Laterale                                    ⌛ 1

🟡 Continuo Sequenza   📜 Missione   ⛓️ Sequenza

EFFETTO RAPIDO
▶️ 1◻️ → 🔁

CONDIZIONE
Giocabile solo dopo una Carta Inizio Sequenza o Continuo Sequenza.

TESTO
Muovi fino a 1 Locazione.
Puoi continuare la Sequenza.

NOTE
Questo Movimento non può aprire Porte Semplici.
```

---

### 12.4 Pressione Coordinata

```text
🟡 Pressione Coordinata                              ⌛ 2

🟡 Continuo Sequenza   📜 Missione   ⛓️ Sequenza

EFFETTO RAPIDO
⏸️ 1❌ / 🤝 2❌ → 🔁

CONDIZIONE
Giocabile solo dopo una Carta Inizio Sequenza o Continuo Sequenza.

TESTO
Effettua un Attacco in Mischia.
Infliggi 1❌.
Se nella tua Locazione è presente almeno 1 Alleato, infliggi invece 2❌.
Puoi continuare la Sequenza.
```

---

### 12.5 Chiusura Brutale

```text
🔴 Chiusura Brutale                                  ⌛ 3

🔴 Fine Sequenza   📜 Missione   ⛓️ Sequenza

EFFETTO RAPIDO
⏸️ 2❌ → ⛔🔁

CONDIZIONE
Giocabile solo durante una Sequenza.

TESTO
Effettua un Attacco in Mischia.
Infliggi 2❌.
Dopo questa Carta, la Sequenza termina.
```

---

## 13. Regole UI per carte con più effetti

Se una carta ha più effetti, usare ordine fisso:

1. Movimento;
2. Attacco;
3. Interazione;
4. Cura;
5. Difesa;
6. Supporto;
7. effetti speciali;
8. rimozione o eliminazione della carta.

Esempio:

```text
EFFETTO RAPIDO
▶️ 1◻️ + ⏸️ 1❌

TESTO
Muovi fino a 1 Locazione.
Poi effettua un Attacco in Mischia e infliggi 1❌.
```

---

## 14. Regole UI per Istantanee

Le Istantanee devono sempre mostrare il trigger prima del testo.

Formato obbligatorio:

```text
TRIGGER
Quando [...]

TESTO
[effetto]
```

Se l’Istantanea interrompe una Sequenza o una Carta Comportamento, deve indicarlo esplicitamente.

Esempio:

```text
TESTO
Annulla quell’Attacco.
La Carta Comportamento del Mostro termina immediatamente.
```

---

## 15. Regole UI per Preparazione / Memoria

Le Carte di Preparazione devono mostrare chiaramente che non producono un effetto immediato.

Formato consigliato:

```text
🧠 Mira Raccolta                                    ⌛ 1

🧠 Preparazione   📜 Missione

EFFETTO RAPIDO
Prossimo ⏸️ +1❌

TESTO
La tua prossima Azione o Carta di Attacco infligge +1❌.
Dopo il prossimo Attacco, scarta questa Carta.
La tua Attivazione termina immediatamente.
```

Badge obbligatorio:

```text
🧠 Preparata
```

---

## 16. Checklist di implementazione

Una Carta Missione è conforme se:

- ha un nome leggibile;
- ha almeno una icona principale;
- mostra sempre il costo in ⌛;
- mostra il tipo di uso;
- mostra l’origine;
- ha un effetto rapido iconico;
- ha un testo completo breve;
- separa trigger, condizioni e note;
- nasconde i dettagli tecnici in sezione collassabile;
- usa sempre gli stessi simboli per le stesse regole;
- non mostra campi interni nella vista normale.

---

## 17. Regola finale di leggibilità

Una carta deve essere comprensibile in due passaggi:

```text
1. Colpo d’occhio: icona + costo + effetto rapido.
2. Conferma: testo breve + trigger/condizioni.
```

Se una carta richiede più di pochi secondi per capire cosa fa, il testo deve essere riscritto o spostato in una nota estesa.
