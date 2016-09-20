// Module for interfacing with PIO

#include "whitecat.h"

#if LUA_USE_PIO

#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Lua/modules/pio.h"
#include "drivers/cpu/cpu.h"
#include "drivers/gpio/gpio.h"

// PIO public constants
#define PIO_DIR_OUTPUT      0
#define PIO_DIR_INPUT       1

// PIO private constants
#define PIO_PORT_OP         0
#define PIO_PIN_OP          1

// ****************************************************************************
// Generic helper functions


// Helper function: pin operations
// Gets the stack index of the first pin and the operation
static int pioh_set_pins(lua_State* L, int stackidx, int op)
{
  int total = lua_gettop(L);
  int i, v, port, pin;

  pio_type pio_masks[PLATFORM_IO_PORTS];
 
  for(i = 0; i < PLATFORM_IO_PORTS; i ++)
    pio_masks[i] = 0;
  
  // Get all masks
  for(i = stackidx; i <= total; i ++)
  {
    v = luaL_checkinteger(L, i);
    port = PORB(v);
    pin = PINB(v);
    if(!platform_pio_has_port(port + 1) || !platform_pio_has_pin(port + 1, pin))
      return luaL_error(L, "invalid pin");
    pio_masks[port] |= 1 << pin;
  }
  
  // Ask platform to execute the given operation
  for(i = 0; i < PLATFORM_IO_PORTS; i ++)
    if(pio_masks[i])
      if(!platform_pio_op(i, pio_masks[i], op))
        return luaL_error(L, "invalid PIO operation");
  return 0;
}

// Helper function: port operations
// Gets the stack index of the first port and the operation (also the mask)
static int pioh_set_ports(lua_State* L, int stackidx, int op, pio_type mask)
{
  int total = lua_gettop(L);
  int i, v, port;
  u32 port_mask = 0;

  // Get all masks
  for(i = stackidx; i <= total; i ++)
  {
    v = luaL_checkinteger(L, i);
    port = PORB(v);
    if(!platform_pio_has_port(port))
      return luaL_error(L, "invalid port");
    port_mask |= (1ULL << port);
  }
  
  // Ask platform to execute the given operation
  for(i = 0; i < PLATFORM_IO_PORTS; i ++)
    if(port_mask & (1ULL << i))
      if(!platform_pio_op(i, mask, op))
        return luaL_error(L, "invalid PIO operation");
  return 0;
}

// ****************************************************************************
// Pin/port helper functions

static int pio_gen_setdir(lua_State *L, int optype) {
  int op = luaL_checkinteger(L, 1);

  if(op == PIO_DIR_INPUT)
    op = optype == PIO_PIN_OP ? PLATFORM_IO_PIN_DIR_INPUT : PLATFORM_IO_PORT_DIR_INPUT;
  else if(op == PIO_DIR_OUTPUT)
    op = optype == PIO_PIN_OP ? PLATFORM_IO_PIN_DIR_OUTPUT : PLATFORM_IO_PORT_DIR_OUTPUT;
  else
    return luaL_error(L, "invalid direction");
  if(optype == PIO_PIN_OP)
    pioh_set_pins(L, 2, op);
  else
    pioh_set_ports(L, 2, op, PLATFORM_IO_ALL_PINS);
  return 0;
}

static int pio_gen_setpull(lua_State *L, int optype) {
  int op = luaL_checkinteger(L, 1);

  if((op != PLATFORM_IO_PIN_PULLUP) &&
      (op != PLATFORM_IO_PIN_PULLDOWN) &&
      (op != PLATFORM_IO_PIN_NOPULL))
    return luaL_error(L, "invalid pull type");
  if(optype == PIO_PIN_OP)
    pioh_set_pins(L, 2, op);
  else
    pioh_set_ports(L, 2, op, PLATFORM_IO_ALL_PINS);
  return 0;
}

static int pio_gen_setval(lua_State *L, int optype, pio_type val, int stackidx) {
  if((optype == PIO_PIN_OP) && (val != 1) && (val != 0)) 
    return luaL_error(L, "invalid pin value");
  if(optype == PIO_PIN_OP)
    pioh_set_pins(L, stackidx, val == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR);
  else
    pioh_set_ports(L, stackidx, PLATFORM_IO_PORT_SET_VALUE, val);
  return 0;
}

// ****************************************************************************
// Pin operations

// Lua: pio.pin.setdir(pio.INPUT | pio.OUTPUT, pin1, pin2, ..., pinn)
static int pio_pin_setdir(lua_State *L)
{
  return pio_gen_setdir(L, PIO_PIN_OP);
}

// Lua: pio.pin.output(pin1, pin2, ..., pinn)
static int pio_pin_output(lua_State *L)
{
  return pioh_set_pins(L, 1, PLATFORM_IO_PIN_DIR_OUTPUT);
}

// Lua: pio.pin.input(pin1, pin2, ..., pinn)
static int pio_pin_input(lua_State *L)
{
  return pioh_set_pins(L, 1, PLATFORM_IO_PIN_DIR_INPUT);
}

// Lua: pio.pin.setpull(pio.PULLUP | pio.PULLDOWN | pio.NOPULL, pin1, pin2, ..., pinn)
static int pio_pin_setpull(lua_State *L)
{
  return pio_gen_setpull(L, PIO_PIN_OP);
}

// Lua: pio.pin.setval(0|1, pin1, pin2, ..., pinn)
static int pio_pin_setval(lua_State *L)
{
  pio_type val = (pio_type)luaL_checkinteger(L, 1);

  return pio_gen_setval(L, PIO_PIN_OP, val, 2);
}

// Lua: pio.pin.sethigh(pin1, pin2, ..., pinn)
static int pio_pin_sethigh(lua_State *L)
{
  return pio_gen_setval(L, PIO_PIN_OP, 1, 1);
}

// Lua: pio.pin.setlow(pin1, pin2, ..., pinn)
static int pio_pin_setlow(lua_State *L)
{
  return pio_gen_setval(L, PIO_PIN_OP, 0, 1);
}

// Lua: pin1, pin2, ..., pinn = pio.pin.getval(pin1, pin2, ..., pinn)
static int pio_pin_getval(lua_State *L)
{
  pio_type value;
  int v, i, port, pin;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++)
  {
    v = luaL_checkinteger(L, i);  
    port = PORB(v);
    pin = PINB(v);
    if(!platform_pio_has_port(port + 1) || !platform_pio_has_pin(port + 1, pin))
      return luaL_error(L, "invalid pin");
    else
    {
      value = platform_pio_op(port, 1 << pin, PLATFORM_IO_PIN_GET) >> pin;
      lua_pushinteger(L, value);
    }
  }
  return total;
}

static int pio_pin_pinnum(lua_State *L)
{
  pio_type value;
  int v, i, port, pin;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++)
  {
    v = luaL_checkinteger(L, i);  
    port = PORB(v);
    pin = PINB(v);
    if(!platform_pio_has_port(port + 1) || !platform_pio_has_pin(port + 1, pin))
      return luaL_error(L, "invalid pin");
    else
    {
      value = cpu_pin_number(v);
      lua_pushinteger(L, value);
    }
  }
  return total;
}

// ****************************************************************************
// Port operations

// Lua: pio.port.setdir(pio.INPUT | pio.OUTPUT, port1, port2, ..., portn)
static int pio_port_setdir(lua_State *L)
{
  return pio_gen_setdir(L, PIO_PORT_OP);
}

// Lua: pio.port.output(port1, port2, ..., portn)
static int pio_port_output(lua_State *L)
{
  return pioh_set_ports(L, 1, PLATFORM_IO_PIN_DIR_OUTPUT, PLATFORM_IO_ALL_PINS);
}

// Lua: pio.port.input(port1, port2, ..., portn)
static int pio_port_input(lua_State *L)
{
  return pioh_set_ports(L, 1, PLATFORM_IO_PIN_DIR_INPUT, PLATFORM_IO_ALL_PINS);
}

// Lua: pio.port.setpull(pio.PULLUP | pio.PULLDOWN | pio.NOPULL, port1, port2, ..., portn)
static int pio_port_setpull(lua_State *L)
{
  return pio_gen_setpull(L, PIO_PORT_OP);
}

// Lua: pio.port.setval(value, port1, port2, ..., portn)
static int pio_port_setval(lua_State *L)
{
  pio_type val = (pio_type)luaL_checkinteger(L, 1);

  return pio_gen_setval(L, PIO_PORT_OP, val, 2);
}

// Lua: pio.port.sethigh(port1, port2, ..., portn)
static int pio_port_sethigh(lua_State *L)
{
  return pio_gen_setval(L, PIO_PORT_OP, PLATFORM_IO_ALL_PINS, 1);
}

// Lua: pio.port.setlow(port1, port2, ..., portn)
static int pio_port_setlow(lua_State *L)
{
  return pio_gen_setval(L, PIO_PORT_OP, 0, 1);
}

// Lua: val1, val2, ..., valn = pio.port.getval(port1, port2, ..., portn)
static int pio_port_getval(lua_State *L)
{
  pio_type value;
  int v, i, port;
  int total = lua_gettop(L);
  
  for(i = 1; i <= total; i ++)
  {
    v = luaL_checkinteger(L, i);  
    port = PORB(v);
    if(!platform_pio_has_port(port))
      return luaL_error(L, "invalid port");
    else
    {
      value = platform_pio_op(port, PLATFORM_IO_ALL_PINS, PLATFORM_IO_PORT_GET_VALUE);
      lua_pushinteger(L, value);
    }
  }
  return total;
}

static int pio_decode(lua_State *L) {
  int code = (int)luaL_checkinteger(L, 1);
  int port = PORB(code);
  int pin  = PINB(code);

  lua_pushinteger(L, port);
  lua_pushinteger(L, pin);

  return 2;
}

static const luaL_Reg pio_pin_map[] = {
    {"setdir", pio_pin_setdir},
    {"output", pio_pin_output},
    {"input", pio_pin_input},
    {"setpull", pio_pin_setpull},
    {"setval", pio_pin_setval},
    {"sethigh", pio_pin_sethigh},
    {"setlow", pio_pin_setlow},
    {"getval", pio_pin_getval},
    {"num", pio_pin_pinnum},
    {NULL, NULL}
};

static const luaL_Reg pio_port_map[] = {
    {"setdir", pio_port_setdir},
    {"output", pio_port_output},
    {"input", pio_port_input},
    {"setpull", pio_port_setpull},
    {"setval", pio_port_setval},
    {"sethigh", pio_port_sethigh},
    {"setlow", pio_port_setlow},
    {"getval", pio_port_getval},
    {NULL, NULL}
};

const luaL_Reg pio_map[] = {
    {"decode", pio_decode},
    {NULL, NULL}
};

LUALIB_API int luaopen_pio(lua_State *L) {
    luaL_newlib(L, pio_map);

    // Set it as its own metatable
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
  
    // Set constants for direction/pullups
    lua_pushinteger(L, PIO_DIR_INPUT);
    lua_setfield(L, -2, "INPUT");

    lua_pushinteger(L, PIO_DIR_OUTPUT);
    lua_setfield(L, -2, "OUTPUT");

    lua_pushinteger(L, PLATFORM_IO_PIN_PULLUP);
    lua_setfield(L, -2, "PULLUP");

    lua_pushinteger(L, PLATFORM_IO_PIN_PULLDOWN);
    lua_setfield(L, -2, "PULLDOWN");

    lua_pushinteger(L, PLATFORM_IO_PIN_NOPULL);
    lua_setfield(L, -2, "NOPULL");

    int port, pin;
    char tmp[6];
  
    // Set constants for port names
    for(port=1;port <= 8; port++) {
        if (platform_pio_has_port(port)) {
            sprintf(tmp,"P%c", platform_pio_port_name(port));
            lua_pushinteger(L, (port << 4));
            lua_setfield(L, -2, tmp);
        }
    }

    // Set constants for pin names
    for(port=1;port <= 8; port++) {
        if (platform_pio_has_port(port)) {
            for(pin=0;pin < 16;pin++) {
                if (platform_pio_has_pin(port, pin)) {   
                    sprintf(tmp,"P%c_%d", platform_pio_port_name(port), pin);
                    lua_pushinteger(L, (port << 4) | pin);
                    lua_setfield(L, -2, tmp);
                }
            }
        }
    }
    
    // Setup the new tables (pin and port) inside pio
    lua_newtable(L);
    luaL_register(L, NULL, pio_pin_map);
    lua_setfield(L, -2, "pin");

    lua_newtable(L);
    luaL_register(L, NULL, pio_port_map);
    lua_setfield(L, -2, "port");

    return 1;
}

#endif