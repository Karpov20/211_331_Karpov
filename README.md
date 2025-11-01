# 211_331_Karpov

## Overview

Qt Widgets application for validating shipment transactions described in JSON files.
Records are rendered in a grid layout; if the calculated hash chain diverges from the
stored value, the affected record and every subsequent one are highlighted in red.

## Building

```
cmake -S . -B build
cmake --build build
```

Run the executable produced in `build/ShipmentLedger.exe`. By default the application
loads `data/transactions_valid.json`.

## Demo Data

Two JSON samples demonstrate the hash validation flow:

- `data/transactions_valid.json` contains a consistent chain of five records.
- `data/transactions_corrupted.json` modifies one record and triggers the red highlight.
