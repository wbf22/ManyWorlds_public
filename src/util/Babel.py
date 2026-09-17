alphabet = "abcdefghijklmnopqrstuvwxyz .,;:'\"!?$"  # example
A = len(alphabet)

def text_to_seed(text):
    number = 0
    for i, char in enumerate(reversed(text)):
        number += alphabet.index(char) * (A ** i)
    return number  # deterministic "seed" for this text

def seed_to_text(seed, alphabet, length):
    A = len(alphabet)
    chars = []
    n = seed
    for _ in range(length):
        index = n % A
        chars.append(alphabet[index])
        n //= A
    return ''.join(reversed(chars))

def seed_to_coordinates(number):
    # Example: map number to library, shelf, wall, book
    books_per_shelf = 100
    shelves_per_wall = 100
    walls_per_library = 100
    
    book_index = number % books_per_shelf
    number //= books_per_shelf
    shelf_index = number % shelves_per_wall
    number //= shelves_per_wall
    wall_index = number % walls_per_library
    number //= walls_per_library
    library_index = number
    return library_index, wall_index, shelf_index, book_index



test_str = "hi this is a test. hi this is a test. hi this is a test. hi this is a test. hi this is a test. hi this is a test. hi this is a test. hi this is a test. hi this is a test. hi this is a test."

seed = text_to_seed(test_str)
print(seed)
back_str = seed_to_text(seed, alphabet, len(test_str))
print(back_str)



# The library of babel just works by converting the string into a number. It's not the reversable random number generator we were hoping for
