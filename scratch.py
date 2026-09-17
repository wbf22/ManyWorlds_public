import os
from pathlib import Path

def delete_import_files(directory):
    """
    Recursively delete all .import files in the given directory.
    """
    directory = Path(directory)

    if not directory.exists():
        print(f"Directory '{directory}' does not exist.")
        return

    deleted_files = 0

    for file_path in directory.rglob("*.import"):
        try:
            file_path.unlink()  # delete the file
            print(f"Deleted: {file_path}")
            deleted_files += 1
        except Exception as e:
            print(f"Failed to delete {file_path}: {e}")

    print(f"Total .import files deleted: {deleted_files}")



def delete_os_files(directory):
    """
    Recursively delete all .os files in the given directory.
    """
    directory = Path(directory)

    if not directory.exists():
        print(f"Directory '{directory}' does not exist.")
        return

    deleted_files = 0

    for file_path in directory.rglob("*.os"):
        try:
            file_path.unlink()  # delete the file
            print(f"Deleted: {file_path}")
            deleted_files += 1
        except Exception as e:
            print(f"Failed to delete {file_path}: {e}")

    print(f"Total .os files deleted: {deleted_files}")

if __name__ == "__main__":
    dir_to_clean = "src"
    # delete_import_files(dir_to_clean)
    delete_os_files(dir_to_clean)
