CC     = gcc
CFLAGS = -Wall -Wextra -std=c11   # Enable most warnings, use C11 standard
LIBS   = -lm                       # Link math library for log2()

TARGET = entropy
SRCS   = main.c entropy.c
OBJS   = $(SRCS:.c=.o)            # Substitution: main.c → main.o, entropy.c → entropy.o

all: $(TARGET)

# Link step — combines object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Compile step — each .c file compiled independently into a .o file
# $< = source file, $@ = target .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Remove compiled output for a clean rebuild
clean:
	rm -f $(OBJS) $(TARGET)