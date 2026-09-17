"""
lldb_unordered_map.py  –  LLDB synthetic children for std::unordered_map/set (libstdc++)

Confirmed working layout (libstdc++, no cached hash):

  _Hash_node_base   (8 bytes)
    _M_nxt          ptr  +0x00

  _Hash_node (inherits _Hash_node_base, NO hash cache):
    _M_v            value_type (std::pair<const K, V>)  @ +0x08

─────────────────────────────────────────────────────────
SETUP  (pick ONE method)
─────────────────────────────────────────────────────────

Method A – Permanent: add to ~/.lldbinit
    command script import /path/to/lldb_unordered_map.py

Method B – Per-session in LLDB / VS Code Debug Console
    command script import /path/to/lldb_unordered_map.py

Method C – VS Code launch.json  (CodeLLDB extension)
    "initCommands": [
        "command script import /path/to/lldb_unordered_map.py"
    ]
─────────────────────────────────────────────────────────

DEBUG BUILD: this version prints diagnostic info to the Debug Console
whenever type resolution or update() fails, instead of failing silently.
Once things work, swap the `except Exception as e: print(...)` blocks
back to `except Exception: pass` to quiet it down again.
─────────────────────────────────────────────────────────
"""

import lldb
import re


# ──────────────────────────────────────────────────────────────────────────────
# Constants
# ──────────────────────────────────────────────────────────────────────────────

# Offset from the _Hash_node_base* to the start of _M_v (the pair<K,V>).
# Confirmed from memory inspection:
#   +0x00  _M_nxt (next pointer, 8 bytes)
#   +0x08  _M_v   (the value — no cached hash in this build)
VALUE_OFFSET = 8


# ──────────────────────────────────────────────────────────────────────────────
# Synthetic provider
# ──────────────────────────────────────────────────────────────────────────────

class UnorderedMapSynthProvider:
    """
    Walks libstdc++'s _Hashtable singly-linked list.

    Starting point: _M_h._M_before_begin._M_nxt  (first real node)
    Each node:      cast node_ptr+VALUE_OFFSET to the map's value_type

    Type resolution strategy:
      GetTemplateArgumentType() is broken in this LLDB build (returns None
      for all args despite GetName() working fine). Instead we:
        1. Read the allocator type name from template arg 4 as a string:
              std::allocator<std::pair<const K, V> >
        2. Strip the outer std::allocator<...> wrapper to get the pair name
        3. Call FindFirstType() with that exact string — confirmed working
    """

    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.count = 0
        self.node_addrs = []
        self.value_type = None

    # ── SBSyntheticValueProvider protocol ────────────────────────────────────

    def num_children(self):
        return self.count

    def get_child_index(self, name):
        try:
            return int(name.lstrip("[").rstrip("]"))
        except Exception:
            return -1

    def get_child_at_index(self, index):
        if index < 0 or index >= self.count or self.value_type is None:
            return None
        try:
            addr = self.node_addrs[index] + VALUE_OFFSET
            return self.valobj.CreateValueFromAddress(
                f"[{index}]", addr, self.value_type
            )
        except Exception as e:
            print(f"[debug] get_child_at_index({index}) failed: {e}")
            return None

    def update(self):
        self.node_addrs = []
        self.count = 0
        self.value_type = None
        try:
            ht = self.valobj.GetChildMemberWithName("_M_h")
            if not ht.IsValid():
                print("[debug] update(): _M_h not valid")
                return False

            # ── element count ────────────────────────────────────────────────
            count_val = ht.GetChildMemberWithName("_M_element_count")
            if not count_val.IsValid():
                print("[debug] update(): _M_element_count not valid")
                return False
            expected = count_val.GetValueAsUnsigned(0)
            # print(f"[debug] update(): expected element count = {expected}")
            if expected == 0:
                return False

            # ── resolve pair<const K, V> type ────────────────────────────────
            self.value_type = self._resolve_value_type()
            if self.value_type is None:
                print("[debug] update(): _resolve_value_type() returned None — bailing before walking list")
                return False
            # print(f"[debug] update(): resolved value_type = {self.value_type.GetName()!r}")

            # ── walk the linked list ─────────────────────────────────────────
            before = ht.GetChildMemberWithName("_M_before_begin")
            if not before.IsValid():
                print("[debug] update(): _M_before_begin not valid")
                return False

            process = self.valobj.GetProcess()
            error = lldb.SBError()

            current_addr = before.GetChildMemberWithName("_M_nxt").GetValueAsUnsigned(0)
            # print(f"[debug] update(): first node addr = {hex(current_addr)}")
            visited = 0

            while current_addr != 0:
                self.node_addrs.append(current_addr)
                visited += 1
                if visited > expected + 1000:   # safety valve
                    print(f"[debug] update(): safety valve tripped at {visited} visits")
                    break
                next_addr = process.ReadPointerFromMemory(current_addr, error)
                if error.Fail() or next_addr == current_addr:
                    if error.Fail():
                        print(f"[debug] update(): ReadPointerFromMemory failed at {hex(current_addr)}: {error}")
                    break
                current_addr = next_addr

            self.count = len(self.node_addrs)
            # print(f"[debug] update(): final count = {self.count}")
        except Exception as e:
            print(f"[debug] update(): exception: {e}")
        return False

    def has_children(self):
        return self.count > 0

    # ── Helpers ───────────────────────────────────────────────────────────────

    def _resolve_value_type(self):
        """
        Extract pair<const K, V> by parsing the allocator type name string.

        Template arg 4 of unordered_map is:
            std::allocator<std::pair<const K, V> >

        GetTemplateArgumentType() is broken in this LLDB build, but
        GetName() works fine, so we strip the wrapper and call FindFirstType.
        """
        # Path 0: pull Value directly off the _Hashtable's own template args.
        # std::_Hashtable<Key, Value, Alloc, ExtractKey, Equal, Hash, RangeHash,
        #                 Unused, RehashPolicy, Traits>
        # Value is pair<const K,V> for maps, or plain K for sets. This is an
        # already-resolved SBType — no name reconstruction, no FindFirstType,
        # so it sidesteps the lookup failures seen in paths 1-3 entirely.
        try:
            ht = self.valobj.GetChildMemberWithName("_M_h")
            if ht.IsValid():
                ht_type = ht.GetType()
                value_type = ht_type.GetTemplateArgumentType(1)
                # print(f"[debug] path0: value_type = "
                #       f"{value_type.GetName() if value_type else None}, "
                #       f"valid={value_type.IsValid() if value_type else False}")
                if value_type and value_type.IsValid():
                    return value_type
        except Exception as e:
            print(f"[debug] path0 exception: {e}")

        try:
            map_type = self.valobj.GetType()

            # arg 4 = allocator<pair<const K, V>>
            arg4_type = map_type.GetTemplateArgumentType(4)
            alloc_name = arg4_type.GetName() if arg4_type else None
            print(f"[debug] path1: alloc_name = {alloc_name!r}")
            if not alloc_name:
                print("[debug] path1: alloc_name empty/None — falling through to path2")
            else:
                # Strip "std::allocator<" prefix and trailing " >" or ">"
                # e.g. "std::allocator<std::pair<const K, V> >" -> "std::pair<const K, V>"
                prefix = "std::allocator<"
                if not alloc_name.startswith(prefix):
                    print(f"[debug] path1: alloc_name does not start with {prefix!r} — falling through to path2")
                else:
                    pair_name = alloc_name[len(prefix):]   # strip prefix
                    pair_name = pair_name.rstrip()
                    if pair_name.endswith(">"):
                        pair_name = pair_name[:-1].rstrip()  # strip trailing " >"
                    # print(f"[debug] path1: pair_name = {pair_name!r}")

                    target = self.valobj.GetTarget()
                    pair_type = target.FindFirstType(pair_name)
                    # print(f"[debug] path1: FindFirstType({pair_name!r}).IsValid() = {pair_type.IsValid()}")
                    if pair_type.IsValid():
                        return pair_type
        except Exception as e:
            print(f"[debug] path1 exception: {e}")

        # Some CodeLLDB/DWARF combinations cannot resolve allocator arg 4,
        # while the key and mapped-value arguments remain available.
        try:
            map_type = self.valobj.GetType()
            key_type = map_type.GetTemplateArgumentType(0)
            mapped_type = map_type.GetTemplateArgumentType(1)
            # print(f"[debug] path2: key_type.IsValid()={key_type.IsValid() if key_type else None}, " f"mapped_type.IsValid()={mapped_type.IsValid() if mapped_type else None}")
            if key_type and mapped_type and key_type.IsValid() and mapped_type.IsValid():
                pair_name = f"std::pair<const {key_type.GetName()}, {mapped_type.GetName()}>"
                # print(f"[debug] path2: pair_name = {pair_name!r}")
                pair_type = self.valobj.GetTarget().FindFirstType(pair_name)
                # print(f"[debug] path2: FindFirstType({pair_name!r}).IsValid() = {pair_type.IsValid()}")
                if pair_type.IsValid():
                    return pair_type
        except Exception as e:
            print(f"[debug] path2 exception: {e}")

        # Final fallback for simple map spellings such as unordered_map<int,int>.
        try:
            map_name = self.valobj.GetType().GetName()
            # print(f"[debug] path3: map_name = {map_name!r}")
            match = re.search(r"unordered_map<\s*([^,>]+)\s*,\s*([^,>]+)", map_name)
            if match:
                pair_name = f"std::pair<const {match.group(1).strip()}, {match.group(2).strip()}>"
                # print(f"[debug] path3: pair_name = {pair_name!r}")
                pair_type = self.valobj.GetTarget().FindFirstType(pair_name)
                # print(f"[debug] path3: FindFirstType({pair_name!r}).IsValid() = {pair_type.IsValid()}")
                if pair_type.IsValid():
                    return pair_type
            else:
                print("[debug] path3: regex did not match map_name")
        except Exception as e:
            print(f"[debug] path3 exception: {e}")

        print("[debug] _resolve_value_type: all three paths failed, returning None")
        return None


# ──────────────────────────────────────────────────────────────────────────────
# Synthetic provider for std::shared_ptr<T> — shows *ptr directly
# ──────────────────────────────────────────────────────────────────────────────

class SharedPtrSynthProvider:
    """
    Shows the pointee's members as direct children, skipping the
    internal _M_ptr / _M_refcount members.
    """

    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.pointee = None

    def num_children(self):
        if self.pointee is None:
            return 0
        return self.pointee.GetNumChildren()

    def get_child_index(self, name):
        if self.pointee is None:
            return -1
        for i in range(self.pointee.GetNumChildren()):
            child = self.pointee.GetChildAtIndex(i)
            if child and child.GetName() == name:
                return i
        return -1

    def get_child_at_index(self, index):
        if self.pointee is None:
            return None
        return self.pointee.GetChildAtIndex(index)

    def update(self):
        self.pointee = None
        try:
            # libc++ (macOS) uses __ptr_, libstdc++ (Linux) uses _M_ptr
            ptr = self.valobj.GetChildMemberWithName("__ptr_")
            if not ptr.IsValid():
                ptr = self.valobj.GetChildMemberWithName("_M_ptr")
            if not ptr.IsValid():
                return False
            addr = ptr.GetValueAsUnsigned(0)
            if addr == 0:
                return False

            pointee_type = ptr.GetType().GetPointeeType()
            if not pointee_type.IsValid():
                return False

            self.pointee = self.valobj.CreateValueFromAddress(
                "*this", addr, pointee_type
            )
        except Exception:
            pass
        return False

    def has_children(self):
        if self.pointee is None:
            return False
        return self.pointee.GetNumChildren() > 0


def shared_ptr_summary(valobj, internal_dict, options=None):
    try:
        # Use the raw (non-synthetic) value so the internal members are visible
        raw = valobj.GetNonSyntheticValue()
        if not raw.IsValid():
            raw = valobj
        ptr = raw.GetChildMemberWithName("__ptr_")
        if not ptr.IsValid():
            ptr = raw.GetChildMemberWithName("_M_ptr")
        if ptr.IsValid():
            addr = ptr.GetValueAsUnsigned(0)
            if addr == 0:
                return "nullptr"
            return f"ptr={addr:#x}"
    except Exception:
        pass
    return "<shared_ptr>"


# ──────────────────────────────────────────────────────────────────────────────
# Summary
# ──────────────────────────────────────────────────────────────────────────────
def unordered_map_summary(valobj, internal_dict, options=None):
    sv = valobj.GetSyntheticValue()
    if sv and sv.IsValid():
        n = sv.GetNumChildren()
        if n > 0:
            return f"size={n}"
    try:
        n = (valobj.GetChildMemberWithName("_M_h")
                   .GetChildMemberWithName("_M_element_count")
                   .GetValueAsUnsigned(0))
        return f"size={n}"
    except Exception:
        return "<no summary>"


# ──────────────────────────────────────────────────────────────────────────────
# Registration
# ──────────────────────────────────────────────────────────────────────────────

def __lldb_init_module(debugger, internal_dict):
    import sys, os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

    cat = "unordered_containers"
    debugger.HandleCommand(f"type category delete {cat}")
    debugger.HandleCommand(f"type category define {cat}")

    patterns = [
        r"^std::unordered_map<.+>$",
        r"^std::unordered_multimap<.+>$",
        r"^std::unordered_set<.+>$",
        r"^std::unordered_multiset<.+>$",
    ]

    module = "lldb_unordered_map"

    for pat in patterns:
        debugger.HandleCommand(
            f'type summary add --category {cat} '
            f'--python-function {module}.unordered_map_summary '
            f'-x "{pat}"'
        )
        debugger.HandleCommand(
            f'type synthetic add --category {cat} '
            f'--python-class {module}.UnorderedMapSynthProvider '
            f'-x "{pat}"'
        )

    # ── shared_ptr formatters ──
    # Anchored at start-of-type so Grid<shared_ptr<...>> doesn't match.
    # Handles both libstdc++ (std::shared_ptr<...>) and Apple libc++
    # (std::__1::shared_ptr<...>).
    debugger.HandleCommand(
        f'type summary add --category {cat} '
        f'--python-function {module}.shared_ptr_summary '
        f'-x "^std::(__1::)?shared_ptr<.+>$"'
    )
    debugger.HandleCommand(
        f'type synthetic add --category {cat} '
        f'--python-class {module}.SharedPtrSynthProvider '
        f'-x "^std::(__1::)?shared_ptr<.+>$"'
    )

    debugger.HandleCommand(f"type category enable {cat}")
    print(f"[lldb_unordered_map] Pretty-printers registered ✓  (libstdc++, VALUE_OFFSET={VALUE_OFFSET})")