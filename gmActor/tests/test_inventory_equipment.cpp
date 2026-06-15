/**
 * @file tests/test_inventory_equipment.cpp
 * @brief Unit tests for InventoryState and EquipmentState.
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. ^
 *       gmActor/items/InventoryState.cpp ^
 *       gmActor/items/EquipmentState.cpp ^
 *       gmActor/tests/test_inventory_equipment.cpp ^
 *       -o bin/exe/test_gmActor_inventory.exe
 */

#include "gmActor/items/InventoryState.hpp"
#include "gmActor/items/EquipmentState.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/core/Errors.hpp"

#include <iostream>
#include <string>

using namespace gmActor;

static int g_pass = 0;
static int g_fail = 0;

static void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
	++g_pass;
}

static void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " -- " << reason << "\n";
	++g_fail;
}

// ── InventoryState tests ──────────────────────────────────────────────────────

static void test_inventory_empty()
{
	const std::string T = "inventory_empty";
	InventoryState inv;
	if (!inv.items().empty()) { fail(T, "should start empty"); return; }
	if (inv.count() != 0)     { fail(T, "count should be 0"); return; }
	pass(T);
}

static void test_inventory_add()
{
	const std::string T = "inventory_add";
	InventoryState inv;
	inv.add("sword_01");
	inv.add("shield_02");

	if (inv.count() != 2)             { fail(T, "expected count 2"); return; }
	if (!inv.contains("sword_01"))    { fail(T, "missing sword_01"); return; }
	if (!inv.contains("shield_02"))   { fail(T, "missing shield_02"); return; }
	pass(T);
}

static void test_inventory_contains_false()
{
	const std::string T = "inventory_contains_false";
	InventoryState inv;
	inv.add("sword_01");
	if (inv.contains("axe_99")) { fail(T, "should not contain axe_99"); return; }
	pass(T);
}

static void test_inventory_remove()
{
	const std::string T = "inventory_remove";
	InventoryState inv;
	inv.add("sword_01");
	inv.add("potion_03");
	inv.remove("sword_01");

	if (inv.contains("sword_01"))   { fail(T, "sword_01 should be gone"); return; }
	if (!inv.contains("potion_03")) { fail(T, "potion_03 should remain"); return; }
	if (inv.count() != 1)           { fail(T, "count should be 1"); return; }
	pass(T);
}

static void test_inventory_remove_unknown_throws()
{
	const std::string T = "inventory_remove_unknown_throws";
	InventoryState inv;
	try
	{
		inv.remove("not_here");
		fail(T, "expected UnknownItemError");
	}
	catch (const UnknownItemError&)
	{
		pass(T);
	}
	catch (...)
	{
		fail(T, "unexpected exception type");
	}
}

// ── EquipmentState tests ──────────────────────────────────────────────────────

static void test_equipment_empty()
{
	const std::string T = "equipment_empty";
	EquipmentState eq;
	if (eq.has_equipped(EquipmentSlot::MAIN_HAND)) { fail(T, "should be empty"); return; }
	if (eq.all_equipped().size() != 0)             { fail(T, "all_equipped should be empty"); return; }
	pass(T);
}

static void test_equipment_equip_and_query()
{
	const std::string T = "equipment_equip_query";
	EquipmentState eq;
	eq.equip(EquipmentSlot::MAIN_HAND, "sword_01");

	if (!eq.has_equipped(EquipmentSlot::MAIN_HAND))   { fail(T, "should be equipped"); return; }
	auto opt = eq.equipped_at(EquipmentSlot::MAIN_HAND);
	if (!opt.has_value())                              { fail(T, "equipped_at returned nullopt"); return; }
	if (*opt != "sword_01")                            { fail(T, "wrong item"); return; }
	pass(T);
}

static void test_equipment_equipped_at_empty_slot()
{
	const std::string T = "equipped_at_empty_slot";
	EquipmentState eq;
	auto opt = eq.equipped_at(EquipmentSlot::OFF_HAND);
	if (opt.has_value()) { fail(T, "expected nullopt for empty slot"); return; }
	pass(T);
}

static void test_equipment_unequip()
{
	const std::string T = "equipment_unequip";
	EquipmentState eq;
	eq.equip(EquipmentSlot::ARMOR, "plate_01");
	eq.unequip(EquipmentSlot::ARMOR);

	if (eq.has_equipped(EquipmentSlot::ARMOR)) { fail(T, "should be unequipped"); return; }
	pass(T);
}

static void test_equipment_unequip_empty_is_noop()
{
	const std::string T = "unequip_empty_is_noop";
	EquipmentState eq;
	eq.unequip(EquipmentSlot::RELIC); // no-op
	if (eq.has_equipped(EquipmentSlot::RELIC)) { fail(T, "should still be empty"); return; }
	pass(T);
}

static void test_equipment_all_equipped()
{
	const std::string T = "all_equipped";
	EquipmentState eq;
	eq.equip(EquipmentSlot::MAIN_HAND, "sword_01");
	eq.equip(EquipmentSlot::TRINKET_1, "ring_02");

	auto all = eq.all_equipped();
	if (all.size() != 2) { fail(T, "expected 2 items"); return; }
	pass(T);
}

static void test_equipment_equip_none_throws()
{
	const std::string T = "equip_none_throws";
	EquipmentState eq;
	try
	{
		eq.equip(EquipmentSlot::NONE, "sword_01");
		fail(T, "expected InvalidEquipmentSlotError");
	}
	catch (const InvalidEquipmentSlotError&)
	{
		pass(T);
	}
	catch (...)
	{
		fail(T, "unexpected exception type");
	}
}

static void test_equipment_double_equip_throws()
{
	const std::string T = "double_equip_throws";
	EquipmentState eq;
	eq.equip(EquipmentSlot::MAIN_HAND, "sword_01");
	try
	{
		eq.equip(EquipmentSlot::MAIN_HAND, "axe_02");
		fail(T, "expected InvalidEquipmentSlotError for occupied slot");
	}
	catch (const InvalidEquipmentSlotError&)
	{
		pass(T);
	}
	catch (...)
	{
		fail(T, "unexpected exception type");
	}
}

static void test_equipment_multiple_slots()
{
	const std::string T = "multiple_slots";
	EquipmentState eq;
	eq.equip(EquipmentSlot::MAIN_HAND, "sword_01");
	eq.equip(EquipmentSlot::OFF_HAND,  "shield_01");
	eq.equip(EquipmentSlot::ARMOR,     "plate_01");
	eq.equip(EquipmentSlot::TRINKET_1, "ring_01");
	eq.equip(EquipmentSlot::TRINKET_2, "ring_02");
	eq.equip(EquipmentSlot::RELIC,     "relic_01");

	if (eq.all_equipped().size() != 6) { fail(T, "expected 6 items"); return; }
	pass(T);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmActor: InventoryState + EquipmentState tests ===\n\n";

	test_inventory_empty();
	test_inventory_add();
	test_inventory_contains_false();
	test_inventory_remove();
	test_inventory_remove_unknown_throws();

	test_equipment_empty();
	test_equipment_equip_and_query();
	test_equipment_equipped_at_empty_slot();
	test_equipment_unequip();
	test_equipment_unequip_empty_is_noop();
	test_equipment_all_equipped();
	test_equipment_equip_none_throws();
	test_equipment_double_equip_throws();
	test_equipment_multiple_slots();

	std::cout << "\n--- Results: " << g_pass << " passed, " << g_fail << " failed ---\n";
	return (g_fail == 0) ? 0 : 1;
}
