/* Ratings conversions by DAV */
/* GNU licensing applies */

#include "stdinclude.h"

#include "command.h"
#include "common.h"
#include "rating_conv.h"
#include "utils.h"

PRIVATE int elo_to_uscf(int elo)
{
  return elo + 100;
}

PRIVATE int uscf_to_elo(int uscf)
{
  return uscf - 100;
}

PRIVATE int bcf_to_elo(int bcf)
{
  return bcf * 8 + 600;
}

PRIVATE int elo_to_bcf(int elo)
{
  return (elo - 600) / 8;
}

PRIVATE int uscf_to_bcf(int uscf)
{
  return (uscf - 700) / 8;
}

PRIVATE int bcf_to_uscf(int bcf)
{
  return bcf * 8 + 700;
}

PRIVATE void printgrades(int p, int elo, int uscf, int bcf)
{
  pprintf(p, "Grading conversion:\n");
  pprintf(p, "  ELO = %d, USCF = %d, BCF = %d\n", elo, uscf, bcf);
}

PUBLIC int com_CONVERT_BCF(int p, param_list param)
{
  int elo = bcf_to_elo(param[0].val.integer);
  int uscf = elo_to_uscf(elo);

  printgrades(p, elo, uscf, param[0].val.integer);
  return COM_OK;
}

PUBLIC int com_CONVERT_ELO(int p, param_list param)
{
  int bcf = elo_to_bcf(param[0].val.integer);
  int uscf = elo_to_uscf(param[0].val.integer);

  printgrades(p, param[0].val.integer, uscf, bcf);
  return COM_OK;
}


PUBLIC int com_CONVERT_USCF(int p, param_list param)
{
  int elo = uscf_to_elo(param[0].val.integer);
  int bcf = elo_to_bcf(elo);

  printgrades(p, elo, param[0].val.integer, bcf);
  return COM_OK;
}

