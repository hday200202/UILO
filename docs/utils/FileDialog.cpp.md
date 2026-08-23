# FileDialog.cpp

`include/utils/FileDialog.cpp`

[← index](../README.md)

## Types

- [FilterSpecs](#filterspecs)

---

### FilterSpecs

The COMDLG_FILTERSPEC array the dialog wants, plus the strings it points into. The API takes bare pointers and keeps no copies, so the backing strings have to outlive the call -- which is the whole reason this is a type and not a function.
