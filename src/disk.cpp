/*! \page License
   KQ is Copyright (C) 2002 by Josh Bolduc

   This file is part of KQ... a freeware RPG.

   KQ is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   KQ is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with KQ; see the file COPYING.  If not, write to
   the Free Software Foundation,
       675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*! \file
 * \brief Functions to load/save to disk
 *
 * These functions are endian independent
 * \author PH
 * \date 20030629
 */

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <tinyxml2.h>
#include <vector>

#include "bounds.h"
#include "disk.h"
#include "heroc.h"
#include "kq.h"
#include "markers.h"
#include "platform.h"
#include "random.h"
#include "shopmenu.h"
#include "sgame.h"

using tinyxml2::XMLElement;
using tinyxml2::XMLDocument;
using std::vector;
using std::copy;
using std::begin;
using std::end;

/*! Iteration helper class.
 * Allows use of C++11's range-based for syntax to iterate
 * through child elements.
 */
typedef std::pair<XMLElement *, const char *> xiterator;
struct xiterable : public std::pair<XMLElement *, const char *> {
  using std::pair<XMLElement *, const char *>::pair;
  xiterator begin() {
    return xiterator(first->FirstChildElement(second), second);
  }
  xiterator end() { return xiterator(nullptr, second); }
};

xiterator &operator++(xiterator &it) {
  it.first = it.first->NextSiblingElement(it.second);
  return it;
}
XMLElement *operator*(xiterator &it) { return it.first; }

xiterable children(XMLElement *parent, const char *tag = nullptr) {
  return xiterable(parent, tag);
}

/** Convert a comma-separated list of ints into a vector.
 * Supplied string can be null or empty (giving an empty list)
 * \param str a string containing the list
 * \returns the numbers in a vector
 */
static vector<int> parse_list(const char *str) {
  vector<int> list;
  while (str && *str) {
    const char *next = strchr(str, ',');
    list.push_back(static_cast<int>(strtol(str, nullptr, 10)));
    if (next) {
      str = next + 1;
    } else {
      str = nullptr;
    }
  }
  return list;
}
/** Generate a comma-separated list from
 * a range specified by two iterators
 * \param begin the start of the range (inclusive)
 * \param end the end of the range (exclusive)
 * \returns a new comma-separated list
 */
template <typename _InputIterator>
static std::string make_list(_InputIterator begin, _InputIterator end) {
  bool first = true;
  std::string ans;
  for (auto i = begin; i != end; ++i) {
    if (first) {
      first = false;
    } else {
      ans += ',';
    }
    ans += std::to_string(static_cast<int>(*i));
  }
  return ans;
}
/* Insert a list of things into the content of an element */
template <typename _InputIterator>
static XMLElement *value_list(XMLElement *elem, _InputIterator begin,
                              _InputIterator end) {
  tinyxml2::XMLText *content =
      elem->GetDocument()->NewText(make_list(begin, end).c_str());
  elem->DeleteChildren();
  elem->InsertFirstChild(content);
  return elem;
}
/** Trim a range.
 * Shorten the given range to exclude any zero elements at the end.
 * \returns a new 'end' iterator
 */
template <typename _InputIterator>
_InputIterator trim_range(_InputIterator begin, _InputIterator end) {
  typedef typename std::iterator_traits<_InputIterator>::value_type vt;
  while (end != begin) {
    _InputIterator n = std::prev(end);
    if (*n != vt()) {
      return end;
    } else {
      end = n;
    }
  }
  return begin;
}
/*! Check if a range is all default.
 * Scan a range, return true if all the elements are
 * the same as their 'default' values (e.g. 0 for integers)
 */
template <typename _InputIterator>
bool range_is_default(_InputIterator first, _InputIterator last) {
  typedef typename std::iterator_traits<_InputIterator>::value_type vt;
  vt v0 = vt();
  while (first != last) {
    if (*first != v0) {
      return false;
    } else {
      ++first;
    }
  }
  return true;
}

static int load_resistances(s_player *s, XMLElement *node) {
  std::fill(std::begin(s->res), std::end(s->res), 0);
  XMLElement *resistances = node->FirstChildElement("resistances");
  if (resistances) {
    auto values = parse_list(resistances->FirstChild()->Value());
    if (!values.empty()) {
      // Gave some, has to be the right number of elements
      if (values.size() == NUM_RES) {
        copy(values.begin(), values.end(), s->res);
      } else {
        TRACE("Wrong number of resistances, expected %d and got %d", NUM_RES,
              values.size());
        Game.program_death("Error loading XML");
      }
    }
  }
  return 0;
}
static int load_spelltypes(s_player *s, XMLElement *node) {
  std::fill(std::begin(s->sts), std::end(s->sts), 0);
  XMLElement *spelltypes = node->FirstChildElement("spelltypes");
  if (spelltypes) {
    auto values = parse_list(spelltypes->FirstChild()->Value());
    if (!values.empty()) {
      if (values.size() == NUM_SPELLTYPES) {
        copy(values.begin(), values.end(), s->sts);
      } else {
        TRACE("Wrong number of spelltypes, expected %d and got %d",
              NUM_SPELLTYPES, values.size());
        Game.program_death("Error loading XML");
      }
    }
  }
  return 0;
}
static int load_spells(s_player *s, XMLElement *node) {
  std::fill(std::begin(s->spells), std::end(s->spells), 0);
  XMLElement *spells = node->FirstChildElement("spells");
  if (spells) {
    auto values = parse_list(spells->FirstChild()->Value());
    if (!values.empty()) {
      if (values.size() == NUM_SPELLS) {
        copy(values.begin(), values.end(), s->spells);
      } else {
        TRACE("Wrong number of spells, expected %d and got %d", NUM_SPELLS,
              values.size());
        Game.program_death("Error loading XML");
      }
    }
  }
  return 0;
}
static int load_equipment(s_player *s, XMLElement *node) {
  std::fill(std::begin(s->eqp), std::end(s->eqp), 0);
  XMLElement *eqp = node->FirstChildElement("equipment");
  if (eqp) {
    auto values = parse_list(eqp->FirstChild()->Value());
    if (!values.empty()) {
      if (values.size() == NUM_EQUIPMENT) {
        copy(values.begin(), values.end(), s->eqp);
      } else {
        TRACE("Wrong number of equipment, expected %d and got %d",
              NUM_EQUIPMENT, values.size());
        Game.program_death("Error loading XML");
      }
    }
  }
  return 0;
}
static int load_attributes(s_player *s, XMLElement *node) {
  XMLElement *attributes = node->FirstChildElement("attributes");
  if (attributes) {
    auto values = parse_list(attributes->FirstChild()->Value());
    if (!values.empty()) {
      if (values.size() == NUM_ATTRIBUTES) {
        copy(values.begin(), values.end(), s->stats);
      } else {
        TRACE("Wrong number of attributes, expected %d and got %d",
              NUM_SPELLTYPES, values.size());
        Game.program_death("Error loading XML");
      }
    }
  }
  return 0;
}
static int load_core_properties(s_player *s, XMLElement *node) {
  XMLElement *properties = node->FirstChildElement("properties");
  if (properties) {
    for (auto property : children(properties, "property")) {
      if (property->Attribute("name", "name")) {
        const char *name = property->Attribute("value");
        strncpy(s->name, name, sizeof(s->name) - 1);
      } else if (property->Attribute("name", "xp")) {
        s->xp = property->IntAttribute("value");
      } else if (property->Attribute("name", "next")) {
        s->next = property->IntAttribute("value");
      } else if (property->Attribute("name", "lvl")) {
        s->lvl = property->IntAttribute("value");
      } else if (property->Attribute("name", "mrp")) {
        s->mrp = property->IntAttribute("value");
      } else if (property->Attribute("name", "hp")) {
        s->hp = property->IntAttribute("value");
      } else if (property->Attribute("name", "mhp")) {
        s->mhp = property->IntAttribute("value");
      } else if (property->Attribute("name", "mp")) {
        s->mp = property->IntAttribute("value");
      } else if (property->Attribute("name", "mmp")) {
        s->mmp = property->IntAttribute("value");
      }
    }
  } else {
    Game.program_death("Core properties missing from XML");
  }
  return 0;
}
static int load_lup(s_player *s, XMLElement *node) {
  XMLElement *elem = node->FirstChildElement("level-up");
  if (elem && !elem->NoChildren()) {
    auto vals = parse_list(elem->FirstChild()->Value());
    copy(vals.begin(), vals.end(), s->lup);
    return 0;
  } else {
    // ???
    return 1;
  }
}
/** Get player (hero) data from an XML node.
 * @param s the structure to write to
 * @param node a node within an XML document.
 * @returns 0 if OK otherwise -1
 */
int load_s_player(s_player *s, XMLElement *node) {
  load_core_properties(s, node);
  load_attributes(s, node);
  load_resistances(s, node);
  load_spelltypes(s, node);
  load_spells(s, node);
  load_equipment(s, node);
  load_lup(s, node);
  return 0;
}

// Helper function - insert a property element.
template <typename T>
static XMLElement *addprop(XMLElement *parent, const char *name, T value) {
  XMLElement *property = parent->GetDocument()->NewElement("property");
  property->SetAttribute("name", name);
  property->SetAttribute("value", value);
  parent->InsertEndChild(property);
  return property;
}

static XMLElement *addprop(XMLElement *parent, const char *name,
                           const std::string &value) {
  return addprop(parent, name, value.c_str());
}
// Store spell info or nothing if all spells are 'zero'
static int store_spells(const s_player *s, XMLElement *node) {
  auto startp = std::begin(s->spells);
  auto endp = std::end(s->spells);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("spells");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 0;
}
static int store_equipment(const s_player *s, XMLElement *node) {
  auto startp = std::begin(s->eqp);
  auto endp = std::end(s->eqp);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("equipment");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 0;
}
static int store_spelltypes(const s_player *s, XMLElement *node) {
  auto startp = std::begin(s->sts);
  auto endp = std::end(s->sts);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("spelltypes");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 0;
}
static int store_resistances(const s_player *s, XMLElement *node) {
  auto startp = std::begin(s->res);
  auto endp = std::end(s->res);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("resistances");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 0;
}
static int store_stats(const s_player *s, XMLElement *node) {
  auto startp = std::begin(s->stats);
  auto endp = std::end(s->stats);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("attributes");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 0;
}

static int store_lup(const s_player *s, XMLElement *node) {
  XMLElement *elem = node->GetDocument()->NewElement("level-up");
  value_list(elem, std::begin(s->lup), std::end(s->lup));
  node->InsertEndChild(elem);
  return 0;
}

struct cstring_less {
  bool operator()(const char *const &a, const char *const &b) const {
    return strcmp(a, b) < 0;
  }
};

static const std::map<const char *, ePIDX, cstring_less> id_lookup = {
    {"sensar", SENSAR},   {"sarina", SARINA},     {"corin", CORIN},
    {"ajathar", AJATHAR}, {"casandra", CASANDRA}, {"temmin", TEMMIN},
    {"ayla", AYLA},       {"noslom", NOSLOM}};
/** Store player inside a node that you supply.
 */
static int save_player(const s_player *s, XMLElement *node) {
  XMLDocument *doc = node->GetDocument();
  XMLElement *hero = doc->NewElement("hero");
  // Crufty way to get the ID of a party member
  ePIDX pid = static_cast<ePIDX>(s - party);
  for (const auto &entry : id_lookup) {
    if (entry.second == pid) {
      hero->SetAttribute("id", entry.first);
      break;
    }
  }
  node->InsertEndChild(hero);
  XMLElement *properties = doc->NewElement("properties");
  hero->InsertFirstChild(properties);
  // Core properties
  addprop(properties, "name", s->name);
  addprop(properties, "hp", s->hp);
  addprop(properties, "xp", s->xp);
  addprop(properties, "next", s->next);
  addprop(properties, "lvl", s->lvl);
  addprop(properties, "mhp", s->mhp);
  addprop(properties, "mp", s->mp);
  addprop(properties, "mmp", s->mmp);
  addprop(properties, "mrp", s->mrp);
  // All other data
  store_stats(s, hero);
  store_resistances(s, hero);
  store_spelltypes(s, hero);
  store_equipment(s, hero);
  store_spells(s, hero);
  store_lup(s, hero);
  return 0;
}

static int load_players(XMLElement *root) {
  XMLElement *heroes_elem = root->FirstChildElement("heroes");
  if (heroes_elem) {
    for (auto hero : children(heroes_elem, "hero")) {
      const char *attr = hero->Attribute("id");
      if (attr) {
        auto it = id_lookup.find(attr);
        if (it != std::end(id_lookup)) {
          load_s_player(&party[it->second], hero);
        }
      }
    }
  } else {
    Game.program_death("Error loading heroes");
  }
  return 1;
}

/** Save all hero data into an XML node.
 * \param heroes array of all heroes
 * \param node a node to save into
 * \returns 0 if error otherwise 1
 */
int save_players(XMLElement *node) {
  XMLDocument *doc = node->GetDocument();
  XMLElement *hs = doc->NewElement("heroes");
  for (const auto &p : party) {
    save_player(&p, hs);
  }
  node->InsertEndChild(hs);
  return 1;
}
// Helper functions for various chunks of data that need saving or loading
static int save_treasures(XMLElement *node) {
  auto startp = std::begin(treasure);
  auto endp = std::end(treasure);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("treasures");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 1;
}
static int load_treasures(XMLElement *node) {
  auto startp = std::begin(treasure);
  auto endp = std::end(treasure);
  std::fill(startp, endp, 0);
  XMLElement *elem = node->FirstChildElement("treasures");
  if (elem && !elem->NoChildren()) {
    auto vs = parse_list(elem->FirstChild()->Value());
    auto it = startp;
    for (auto &v : vs) {
      if (it == endp) {
        // Too much data supplied...
        Game.program_death("Error while loading treasures: More entries for treasures in saved game than there should be!");
      }
      *it++ = v;
    }
  }
  return 1;
}
static int save_progress(XMLElement *node) {
  auto startp = std::begin(progress);
  auto endp = std::end(progress);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("progress");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 1;
}
static int load_progress(XMLElement *node) {
  auto startp = std::begin(progress);
  auto endp = std::end(progress);
  std::fill(startp, endp, 0);
  XMLElement *elem = node->FirstChildElement("progress");
  if (elem && !elem->NoChildren()) {
    auto vs = parse_list(elem->FirstChild()->Value());
    auto it = startp;
    for (auto &v : vs) {
      if (it == endp) {
        // Too much data supplied...
        Game.program_death("Too much data supplied");
      }
      *it++ = v;
    }
  }
  return 1;
}
static int save_save_spells(XMLElement *node) {
  auto startp = std::begin(save_spells);
  auto endp = std::end(save_spells);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("save-spells");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 1;
}
static int load_save_spells(XMLElement *node) {
  auto startp = std::begin(save_spells);
  auto endp = std::end(save_spells);
  std::fill(startp, endp, 0);
  XMLElement *elem = node->FirstChildElement("save-spells");
  if (elem && !elem->NoChildren()) {
    auto vs = parse_list(elem->FirstChild()->Value());
    auto it = startp;
    for (auto &v : vs) {
      if (it == endp) {
        // Too much data supplied...
        Game.program_death("Too much data supplied");
      }
      *it++ = v;
    }
  }
  return 1;
}
static int save_specials(XMLElement *node) {
  auto startp = std::begin(player_special_items);
  auto endp = std::end(player_special_items);
  if (!range_is_default(startp, endp)) {
    XMLElement *elem = node->GetDocument()->NewElement("special");
    value_list(elem, startp, endp);
    node->InsertEndChild(elem);
  }
  return 1;
}
static int load_specials(XMLElement *node) {
  auto startp = std::begin(player_special_items);
  auto endp = std::end(player_special_items);
  std::fill(startp, endp, 0);
  XMLElement *elem = node->FirstChildElement("special");
  if (elem && !elem->NoChildren()) {
    auto vs = parse_list(elem->FirstChild()->Value());
    auto it = startp;
    for (auto &v : vs) {
      if (it == endp) {
        // Too much data supplied...
        Game.program_death("Too much data supplied");
      }
      *it++ = v;
    }
  }
  return 1;
}
static int save_global_inventory(XMLElement *node) {
  XMLDocument *doc = node->GetDocument();
  XMLElement *inventory = doc->NewElement("inventory");
  for (auto &item : g_inv) {
    if (item.quantity > 0) {
      XMLElement *item_elem = doc->NewElement("item");
      item_elem->SetAttribute("id", item.item);
      item_elem->SetAttribute("quantity", item.quantity);
      inventory->InsertEndChild(item_elem);
    }
  }
  node->InsertEndChild(inventory);
  return 1;
}
static int load_global_inventory(XMLElement *node) {
  for (auto &item : g_inv) {
    item.item = 0;
    item.quantity = 0;
  }
  XMLElement *inventory = node->FirstChildElement("inventory");
  if (inventory) {
    auto gptr = g_inv;
    for (auto item : children(inventory, "item")) {
      gptr->item = item->IntAttribute("id");
      gptr->quantity = item->IntAttribute("quantity");
      ++gptr;
    }
  }
  return 0;
}
static int save_shop_info(XMLElement *node) {
  bool visited = false;
  // Check if any shops have been visited
  for (int i = 0; i < num_shops; ++i) {
    if (shops[i].time > 0) {
      visited = true;
      break;
    }
  }
  // If so, we've got something to save.
  if (visited) {
    XMLDocument *doc = node->GetDocument();
    XMLElement *shops_elem = doc->NewElement("shops");
    for (int i = 0; i < num_shops; ++i) {
      s_shop &shop = shops[i];
      if (shop.time > 0) {
        XMLElement *shop_elem = doc->NewElement("shop");
        shop_elem->SetAttribute("id", i);
        shop_elem->SetAttribute("time", shop.time);
        value_list(shop_elem, std::begin(shop.items_current),
                   std::end(shop.items_current));
        shops_elem->InsertEndChild(shop_elem);
      }
    }
    node->InsertEndChild(shops_elem);
  }
  return 1;
}
static int load_shop_info(XMLElement *node) {
  for (auto &shop : shops) {
    shop.time = 0;
    std::fill(std::begin(shop.items_current), std::end(shop.items_current), 0);
  }
  XMLElement *shops_elem = node->FirstChildElement("shops");
  if (shops_elem) {
    for (auto el : children(shops_elem, "shop")) {
      int index = el->IntAttribute("id");
      auto items = parse_list(el->FirstChild()->Value());
      s_shop &shop = shops[index];
      shop.time = el->IntAttribute("time");
      int item_index = 0;
      for (auto &item : items) {
        if (item_index < SHOPITEMS) {
          shop.items_current[item_index] = item;
          ++item_index;
        }
      }
    }
  }
  return 1;
}

static int save_general_props(XMLElement *node) {
  XMLElement *properties = node->GetDocument()->NewElement("properties");
  s_sgstats stats = s_sgstats::get_current();
  addprop(properties, "gold", stats.gold);
  addprop(properties, "time", stats.time);
  addprop(properties, "mapname", Game.GetCurmap());
  addprop(properties, "mapx", g_ent[0].tilex);
  addprop(properties, "mapy", g_ent[0].tiley);
  auto pbegin = std::begin(pidx);
  auto pend = std::next(pbegin, numchrs);
  addprop(properties, "party", make_list(pbegin, pend));
  addprop(properties, "random-state", kqrandom->kq_get_random_state());
  // Save-Game Stats - id, level, hp (as a % of mhp), mp% for each member of the
  // party
  vector<int> sgs;
  for (int i = 0; i < stats.num_characters; ++i) {
    auto &chr = stats.characters[i];
    sgs.push_back(chr.id);
    sgs.push_back(chr.level);
    sgs.push_back(chr.hp);
    sgs.push_back(chr.mp);
  }
  addprop(properties, "sgstats", make_list(sgs.begin(), sgs.end()));
  node->InsertEndChild(properties);
  return 1;
}
static int load_general_props(XMLElement *node) {
  XMLElement *properties = node->FirstChildElement("properties");
  if (properties) {
    for (auto property : children(properties, "property")) {
      if (property->Attribute("name", "gold")) {
        gp = property->IntAttribute("value");
      } else if (property->Attribute("name", "random-state")) {
        std::string state = property->Attribute("value");
        kqrandom->kq_set_random_state(state);
      } else if (property->Attribute("name", "time")) {
        int tt = property->IntAttribute("value");
        kmin = tt % 60;
        khr = (tt - kmin) / 60;
      } else if (property->Attribute("name", "mapname")) {
        Game.SetCurmap(property->Attribute("value"));
      } else if (property->Attribute("name", "mapx")) {
        g_ent[0].tilex = property->IntAttribute("value");
      } else if (property->Attribute("name", "mapy")) {
        g_ent[0].tiley = property->IntAttribute("value");
      } else if (property->Attribute("name", "party")) {
        auto pps = parse_list(property->Attribute("value"));
        auto it = pps.begin();
        numchrs = 0;
        for (int i = 0; i < MAXCHRS; ++i) {
          if (it != pps.end()) {
            pidx[i] = static_cast<ePIDX>(*it++);
            g_ent[i].eid = pidx[i];
            g_ent[i].active = 1;
            ++numchrs;
          } else {
            pidx[i] = PIDX_UNDEFINED;
            g_ent[i].active = 0;
          }
        }
      }
      // Don't need to restore anything from <sgstats>
    }
  }
  return 0;
}
/** Save everything into a node
 */
int save_game_xml(XMLElement *node) {
  node->SetAttribute("version", "93");
  save_general_props(node);
  save_players(node);
  save_treasures(node);
  save_progress(node);
  save_save_spells(node);
  save_specials(node);
  save_global_inventory(node);
  save_shop_info(node);
  return 1;
}

int save_game_xml(const char *filename) {
  XMLDocument doc;
  XMLElement *save = doc.NewElement("save");
  int k = save_game_xml(save);
  doc.InsertFirstChild(doc.NewDeclaration());
  doc.InsertEndChild(save);
  doc.SaveFile(filename);
  return k;
}

/** Load everything from a node
 */
int load_game_xml(XMLElement *node) {
  load_general_props(node);
  load_players(node);
  load_treasures(node);
  load_progress(node);
  load_save_spells(node);
  load_specials(node);
  load_global_inventory(node);
  load_shop_info(node);
  return 1;
}

/** Load everything from a file */
int load_game_xml(const char *filename) {
  XMLDocument doc;
  doc.LoadFile(filename);
  if (!doc.Error()) {
    return load_game_xml(doc.RootElement());
  } else {
    TRACE("%s(%d)\n%s\n%s", doc.ErrorName(), doc.ErrorID(), doc.GetErrorStr1(),
          doc.GetErrorStr2());
    Game.program_death("Unable to load XML file");
  }
  return 0;
}

static void printprop(tinyxml2::XMLPrinter &out, const char *name, int value) {
  out.OpenElement("property");
  out.PushAttribute("name", name);
  out.PushAttribute("value", value);
  out.CloseElement();
}
static void printprop(tinyxml2::XMLPrinter &out, const char *name,
                      const char *value) {
  out.OpenElement("property");
  out.PushAttribute("name", name);
  out.PushAttribute("value", value);
  out.CloseElement();
}
static int save_s_fighter(tinyxml2::XMLPrinter &out, const s_fighter &f) {
  out.OpenElement("fighter");
  out.PushAttribute("id", f.name);
  out.OpenElement("properties");
  printprop(out, "name", f.name);
  printprop(out, "xp", f.xp);
  printprop(out, "gp", f.gp);
  printprop(out, "lvl", f.lvl);
  printprop(out, "cx", f.cx);
  printprop(out, "cy", f.cy);
  printprop(out, "cw", f.cw);
  printprop(out, "cl", f.cl);
  printprop(out, "hp", f.hp);
  printprop(out, "mhp", f.mhp);
  printprop(out, "mp", f.mp);
  printprop(out, "mmp", f.mmp);
  printprop(out, "dip", f.dip);
  printprop(out, "defeat-item-common", f.defeat_item_common);
  printprop(out, "defeat-item-rare", f.defeat_item_rare);
  printprop(out, "steal-item-common", f.steal_item_common);
  printprop(out, "steal-item-rare", f.steal_item_rare);
  printprop(out, "facing", f.facing);
  printprop(out, "aframe", f.aframe);
  printprop(out, "crit", f.crit);
  printprop(out, "defend", f.defend);
  printprop(out, "csmem", f.csmem);
  printprop(out, "ctmem", f.ctmem);
  printprop(out, "current-weapon-type", f.current_weapon_type);
  printprop(out, "welem", f.welem);
  printprop(out, "unl", f.unl);
  printprop(out, "aux", f.aux);
  printprop(out, "bonus", f.bonus);
  printprop(out, "bstat", f.bstat);
  printprop(out, "mrp", f.mrp);
  out.CloseElement(/*properties*/);
  out.OpenElement("stats");
  out.PushText(make_list(std::begin(f.stats), std::end(f.stats)).c_str());
  out.CloseElement(/*stats*/);
  out.OpenElement("res");
  out.PushText(make_list(std::begin(f.res), std::end(f.res)).c_str());
  out.CloseElement(/*res*/);
  out.OpenElement("sts");
  out.PushText(make_list(std::begin(f.sts), std::end(f.sts)).c_str());
  out.CloseElement(/*sts*/);
  out.OpenElement("ai");
  out.PushText(make_list(std::begin(f.ai), std::end(f.ai)).c_str());
  out.CloseElement();
  out.OpenElement("aip");
  out.PushText(make_list(std::begin(f.aip), std::end(f.aip)).c_str());
  out.CloseElement();
  out.OpenElement("atrack");
  out.PushText(make_list(std::begin(f.atrack), std::end(f.atrack)).c_str());
  out.CloseElement();
  out.OpenElement("imb");
  vector<int> imb{f.imb_s, f.imb_a, f.imb[0], f.imb[1]};
  out.PushText(make_list(imb.begin(), imb.end()).c_str());
  out.CloseElement(/*imb*/);
  out.CloseElement(/*fighter*/);
  return 0;
}

int save_fighters(const char *filename, s_fighter *fighters, int count) {
  FILE *f = fopen(filename, "wb");
  if (f) {
    tinyxml2::XMLPrinter out(f);
    out.PushHeader(false, true);
    out.OpenElement("fighters");
    for (int i = 0; i < count; ++i) {
      s_fighter &fighter = fighters[i];
      save_s_fighter(out, fighter);
    }
    out.CloseElement();
    fclose(f);
  }
  return 0;
}

/*! Load an XML file but only to get the stats out of it.
 * \param filename the file name
 * \param stats where to put the stats
 * \returns 0 if OK, otherwise 1
 */
int load_stats_only(const char *filename, s_sgstats &stats) {
  XMLDocument doc;
  doc.LoadFile(filename);
  if (!doc.Error()) {
    XMLElement *properties = doc.RootElement()->FirstChildElement("properties");
    if (properties) {
      stats.num_characters = 0;
      stats.gold = 0;
      stats.time = 0;
      for (auto property : children(properties, "property")) {
        if (property->Attribute("name", "gold")) {
          stats.gold = property->IntAttribute("value");
        } else if (property->Attribute("name", "time")) {
          stats.time = property->IntAttribute("value");
        } else if (property->Attribute("name", "sgstats")) {
          auto sg = parse_list(property->Attribute("value"));
          auto iter = begin(sg);
          while (iter != end(sg)) {
            auto &chr = stats.characters[stats.num_characters++];
            chr.id = *iter++;
            chr.level = *iter++;
            chr.hp = *iter++;
            chr.mp = *iter++;
          }
        }
      }
      return 0;
    }
  } else {
    TRACE("%s(%d)\n%s\n%s", doc.ErrorName(), doc.ErrorID(), doc.GetErrorStr1(),
          doc.GetErrorStr2());
  }
  return 1;
}

/*! Get the save-game stats that apply to the current state.
* \returns a structure containing the stats;
*/
s_sgstats s_sgstats::get_current() {
  s_sgstats stats;
  stats.gold = gp;
  stats.time = khr * 60 + kmin;
  stats.num_characters = numchrs;
  for (auto i = 0U; i < numchrs; ++i) {
    auto &chr = stats.characters[i];
    chr.id = pidx[i];
    auto &pp = party[chr.id];
    chr.hp = pp.mhp > 0 ? pp.hp * 100 / pp.mhp : 0;
    chr.mp = pp.mmp > 0 ? pp.mp * 100 / pp.mmp : 0;
    chr.level = pp.lvl;
  }
  return stats;
}
