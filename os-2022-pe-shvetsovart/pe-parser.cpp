#include <cmath>
#include <iostream>

int first_bit(int x) {
  for (int i = 0; i < 5; ++i) {
    x |= x >> (2 << i);
  }
  return x - (x >> 1);
}

int is_pe_format(char **argv) {
  FILE *f = fopen(argv[2], "rb");

  if (f == nullptr) {
    std::cout << "Not PE\n";
    return 1;
  }

  fseek(f, 0x3C, SEEK_SET);
  uint8_t sign_pos_bytes[4];
  size_t bytes_read = fread(sign_pos_bytes, 1, 4, f);
  if (bytes_read != 4) {
    std::cout << "Not PE\n";
    return 1;
  }

  int pos = 0;
  for (int i = 0; i < 4; i++) {
    pos += sign_pos_bytes[i] * (int)pow(16, 2 * i);
  }

  fseek(f, pos, SEEK_SET);
  uint8_t sign[4];
  bytes_read = fread(sign, 1, 4, f);
  if (bytes_read != 4) {
    std::cout << "Not PE\n";
    return 1;
  }
  fclose(f);

  if (sign[0] == 'P' && sign[1] == 'E' && sign[2] == '\0' && sign[3] == '\0') {
    std::cout << "PE\n";
  } else {
    std::cout << "Not PE\n";
    return 1;
  }

  return 0;
}

int import_functions(char **argv) {
  FILE *f = fopen(argv[2], "rb");

  uint8_t header[4];

  fseek(f, 0x3C, SEEK_SET);
  fread(header, 1, 4, f);

  int pos = 0;
  for (int i = 0; i < 4; i++) {
    pos += header[i] * (int)pow(16, 2 * i);
  }

  fseek(f, pos + 24 + 0x78, SEEK_SET);
  uint8_t import_table_rva[4];
  fread(import_table_rva, 1, 4, f);

  int import_raw, shift_cnt = 0, sec_rva, sec_raw;
  while (true) {
    fseek(f, shift_cnt * 40 + pos + 24 + 240 + 0x8, SEEK_SET);
    uint8_t section_virtual_size[4];
    fread(section_virtual_size, 1, 4, f);

    fseek(f, shift_cnt * 40 + pos + 24 + 240 + 0xC, SEEK_SET);
    uint8_t section_rva[4];
    fread(section_rva, 1, 4, f);

    fseek(f, shift_cnt * 40 + pos + 24 + 240 + 0x14, SEEK_SET);
    uint8_t section_raw[4];
    fread(section_raw, 1, 4, f);

    int imp_tbl_rva = 0, sec_vir_sz = 0;
    sec_rva = 0, sec_raw = 0;
    for (int i = 0; i < 4; i++) {
      imp_tbl_rva += import_table_rva[i] * (int)pow(16, 2 * i);
      sec_rva += section_rva[i] * (int)pow(16, 2 * i);
      sec_vir_sz += section_virtual_size[i] * (int)pow(16, 2 * i);
      sec_raw += section_raw[i] * (int)pow(16, 2 * i);
    }

    if (imp_tbl_rva >= sec_rva && imp_tbl_rva <= (sec_rva + sec_vir_sz)) {
      import_raw = sec_raw + imp_tbl_rva - sec_rva;
      break;
    }

    shift_cnt++;
  }

  shift_cnt = 0;
  while (true) {
    fseek(f, import_raw + shift_cnt * 20, SEEK_SET);
    uint8_t func_rva[4];
    fread(func_rva, 1, 4, f);

    fseek(f, import_raw + 0xC + shift_cnt * 20, SEEK_SET);
    uint8_t lib_rva[4];
    fread(lib_rva, 1, 4, f);

    bool is_empty = true;
    for (unsigned char c : lib_rva) {
      if (c != 0x00) {
        is_empty = false;
        break;
      }
    }

    if (is_empty) {
      break;
    }

    int lib_pos = 0, func_pos = 0;
    for (int i = 0; i < 4; i++) {
      lib_pos += lib_rva[i] * (int)pow(16, 2 * i);
      func_pos += func_rva[i] * (int)pow(16, 2 * i);
    }

    lib_pos = sec_raw + lib_pos - sec_rva;
    func_pos = sec_raw + func_pos - sec_rva;
    fseek(f, lib_pos, SEEK_SET);

    char lib_name_char[1];
    while (true) {
      fread(lib_name_char, 1, 1, f);
      if (lib_name_char[0] == 0) {
        break;
      }
      std::cout << lib_name_char[0];
    }
    std::cout << "\n";

    int func_shift_cnt = 0;
    while (true) {
      fseek(f, func_pos + func_shift_cnt * 8, SEEK_SET);
      uint8_t data[8];
      fread(data, 1, 8, f);

      is_empty = true;
      for (unsigned char c : data) {
        if (c != 0) {
          is_empty = false;
          break;
        }
      }

      if (is_empty) {
        break;
      }

      int data_pos = 0;
      for (int i = 0; i < 4; i++) {
        data_pos += data[i] * (int)pow(16, 2 * i);
      }

      if (first_bit(data_pos) == 1) {
        func_shift_cnt++;
        continue;
      }

      int res_list = data_pos + sec_raw - sec_rva + 2;
      fseek(f, res_list, SEEK_SET);

      char func_name_char[1];
      std::cout << "    ";
      while (true) {
        fread(func_name_char, 1, 1, f);

        if (func_name_char[0] == 0) {
          break;
        }

        std::cout << func_name_char[0];
      }
      std::cout << "\n";

      func_shift_cnt++;
    }

    shift_cnt++;
  }

  fclose(f);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    return 1;
  }

  int res = 0;
  std::string task = argv[1];

  if (task == "is-pe") {
    res = is_pe_format(argv);
  } else if (task == "import-functions") {
    res = import_functions(argv);
  }

  if (res) {
    return 1;
  } else {
    return 0;
  }
}
