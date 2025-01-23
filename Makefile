# Ustawienia kompilatora
CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -pedantic

# Ścieżki
SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = include

# Pliki źródłowe
PRACOWNIK_TECHNICZNY = $(SRC_DIR)/pracownik_techniczny.c
RESOURCES = $(SRC_DIR)/resources.c
KIBIC = $(SRC_DIR)/kibic.c
KIEROWNIK = $(SRC_DIR)/kierownik_stadionu.c

# Pliki wynikowe
KIBIC_BIN = $(BIN_DIR)/kibic
PRACOWNIK_BIN = $(BIN_DIR)/pracownik_techniczny
KIEROWNIK_BIN = $(BIN_DIR)/kierownik_stadionu

# Cele główne
all: $(KIBIC_BIN) $(PRACOWNIK_BIN) $(KIEROWNIK_BIN)

# Kompilacja kibic
$(KIBIC_BIN): $(KIBIC) $(RESOURCES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@

# Kompilacja pracownik_techniczny
$(PRACOWNIK_BIN): $(PRACOWNIK_TECHNICZNY) $(RESOURCES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@

# Kompilacja kierownik_stadionu
$(KIEROWNIK_BIN): $(KIEROWNIK) $(RESOURCES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@

# Uruchamianie programów w osobnych terminalach
run: all
	@echo "Uruchamianie pracownik_techniczny w osobnym terminalu..."
	gnome-terminal -- bash -c "./$(BIN_DIR)/pracownik_techniczny; exec bash" &
	@echo "Uruchamianie kierownik_stadionu w osobnym terminalu..."
	gnome-terminal -- bash -c "./$(BIN_DIR)/kierownik_stadionu; exec bash" &

# Czyszczenie
clean:
	@echo "Usuwanie plików wykonywalnych..."
	rm -rf $(BIN_DIR)

.PHONY: all clean run
