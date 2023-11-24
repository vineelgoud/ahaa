#include "host.h"
#include <env_time.h>
//#include "sd2_0.h" /* Include SD2.0 specific register defines */
namespace
{
    char buffer[1024];
    bool disableLog = false;
}
using namespace std;
coreutil::Logger HostLogger::mLogger("HOSTLOG");

/*************************************************/
void host_errormsg(const char *lpOutputString, ...)
{
    va_list args;
    va_start(args, lpOutputString);
    vsnprintf(buffer, sizeof(buffer), lpOutputString, args);
    va_end(args);

    HOST_LOG_ERROR() << buffer;
}
/*************************************************/


/*************************************************/
void host_logmsg(const char *lpOutputString, ...)
{
    if (disableLog)
    {
        return;
    }

    va_list args;
    va_start(args, lpOutputString);
    vsnprintf(buffer, sizeof(buffer), lpOutputString, args);
    va_end(args);

    HOST_LOG_DEBUG() << buffer;
}
/*************************************************/




/*************************************************/
void host_usermsg(const char *lpOutputString, ...)
{
    va_list args;
    va_start(args, lpOutputString);
    vsnprintf(buffer, sizeof(buffer), lpOutputString, args);
    va_end(args);

    HOST_LOG_DEBUG() << buffer;
}
/*************************************************/



/*************************************************/
void host_hostmsg(const char *lpOutputString, ...)
{
    va_list args;
    va_start(args, lpOutputString);
    vsnprintf(buffer, sizeof(buffer), lpOutputString, args);
    va_end(args);

    HOST_LOG_INFO() << buffer;
}
/*************************************************/



/*************************************************/
/* void host_disablelogmsg(void)
{
    disableLog = true;
} */
/*************************************************/
/* void host_setlogbasename(const char *pszBaseName)
{
} */
/*************************************************/



/*************************************************/
void host_closelog(void)
{
    disableLog = false;
}
/*************************************************/


_host *did_host = NULL;
#if SD_PCI
/********************************* Host Object *******************************/
_host::_host(uint32_t pci_bus, uint32_t pci_device, uint32_t pci_function)
{
	
	
	HOST_LOG_DEBUG() << "Creating PCI device for SDXC" << std::endl; 
	create_pci_device( pci_bus,  pci_device,  pci_function);
	p_pci_handler = new _pci_handler;
		
		
	p_card		  = new _card(0,0);
	
}
#endif


/*************************************************/
/*************************************************/
#if SD_MMIO
/********************************* Host Object for MMIO SPACE*******************************/
_host::_host()
{
		
	p_card		  = new _card(0,0);
	
}
#endif


/*************************************************/


/*************************************************/
_host::~_host()
{
	#if SD_PCI
		delete p_pci_handler;
	#endif
	//delete_pci_device();
}
/*************************************************/



/*************************************************/
bool _host::reset(uint8_t reset_type, uint32_t device, uint32_t slot)
{
  _reg reset_register;
  uint32_t try_loop = 10;
  if(reset_type > SW_DAT_LINE_RESET)
  return false;
  
  SET_SD_REGISTER_WRITE(&reset_register,HOST_SOFTWARE_RESET,device,slot,(1 << reset_type));

  if(!write_host_reg(&reset_register))
  return false;
   
  do{
	  env::SleepUs(2);
	  if(!read_host_reg(&reset_register))
	  return false;
	  if( !((reset_register.sd_reg.value)&(1 << reset_type)))
	  return true;
	}while(try_loop--);

  return false;  
}
/*************************************************/





/*************************************************/
bool _host::detect(uint32_t device, uint32_t slot)
{
  _reg prsnt_state_register;
  reg32 present_state;
  SET_SD_REGISTER_READ(&prsnt_state_register,HOST_PRESENT_STATE,device,slot);

  if(!read_host_reg(&prsnt_state_register))
  return false;
 // host_logmsg("Present State Register : %X\n", prsnt_state_register.sd_reg.value); 
  //CORE_LOG_DEBUG(logger()) << "Present State Register" << std::hex << prsnt_state_register.sd_reg.value << std::endl;
  HOST_LOG_DEBUG() << "Present State Register" << std::hex << prsnt_state_register.sd_reg.value << std::endl;
  present_state.reg_val = prsnt_state_register.sd_reg.value  ;
  
  return(GET_CARD_INSERTED(present_state)? true : false);
}
/*************************************************/





/*************************************************/
bool _host::writeprotected(uint32_t device, uint32_t slot)
{
  _reg prsnt_state_register;
  reg32 present_state;
  SET_SD_REGISTER_READ(&prsnt_state_register,HOST_PRESENT_STATE,device,slot);

  if(!read_host_reg(&prsnt_state_register))
  return false;
   
  present_state.reg_val = prsnt_state_register.sd_reg.value  ;
  
  return(GET_CARD_PROTECTED(present_state)? false : true);
}
/*************************************************/



/*************************************************/
bool _host::set_voltage(uint8_t voltage, uint32_t device, uint32_t slot)
{
  _reg pwrctrl_register;
  reg8 pwrctrl;
  pwrctrl.reg_val = voltage;
  pwrctrl.bit.b0  = 0;
  SET_SD_REGISTER_WRITE(&pwrctrl_register,HOST_POWER_CONTROL,device,slot,pwrctrl.reg_val);

  if(!write_host_reg(&pwrctrl_register))
    return false;
  return true;
}
/*************************************************/



/*************************************************/

bool _host::power(bool state,uint32_t device, uint32_t slot)
{
  _reg pwrctrl_register;
  reg8 pwrctrl;

  SET_SD_REGISTER_READ(&pwrctrl_register,HOST_POWER_CONTROL,device,slot);
  if(!read_host_reg(&pwrctrl_register))
    return false;  

  pwrctrl.reg_val = pwrctrl_register.sd_reg.value;
  pwrctrl.bit.b0 = (state == true) ? 1 : 0;
  SET_SD_REGISTER_WRITE(&pwrctrl_register,HOST_POWER_CONTROL,device,slot,pwrctrl.reg_val);

  if(!write_host_reg(&pwrctrl_register))
    return false;
	
  return true;
}
/*************************************************/





/*************************************************/
bool _host::host_init(uint32_t device, uint32_t slot)
{
  //bool result = true;
  _reg sd_register;
  reg32 capability;
  //uint8_t new_reg_val = 0x00;

  reset(SW_ALL_LINE_RESET,device,slot);

  power(false, device,  slot);
  env::SleepMs(50);   
  power(true, device,  slot);	
  
  SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES,device,slot);
  if(!read_host_reg(&sd_register))
    return false; 

    capability.reg_val = sd_register.sd_reg.value;

  if(GET_VOLTAGE_SUPPORT_3_3(capability))
  {
    set_voltage(SD_VOLTAGE_3_3,device, slot);
  }
  else if(GET_VOLTAGE_SUPPORT_3_0(capability))
  {
    set_voltage(SD_VOLTAGE_3_0,device, slot);  
  }
  else if(GET_VOLTAGE_SUPPORT_1_8(capability))
  {
    set_voltage(SD_VOLTAGE_1_8,device, slot);
  }
  //result = power(true,device, slot);
  if(!power(true,device, slot))
	  return false; 
  
  //result = set_clock(device, slot , DEFAULT_FREQUENCY , DIVIDED_CLK_SELECTION_MODE); 
   if(!set_clock(device, slot , DEFAULT_FREQUENCY , DIVIDED_CLK_SELECTION_MODE))
	   return false;
 
  if(!int_status_enable(device , slot, INT_STATUS_ENABLE))
    return false;  

  if(!int_signal_enable(device , slot, INT_SIGNAL_ENABLE))
    return false;  	
 
  return true;
   
}
/*************************************************/



/*************************************************/
void init_data_structure(_init_status *init_status)
{
//Init host status
init_status->host.version 			= 0xFF;
init_status->host.voltage_switch 	= false;
init_status->host.ddr50  			= false;
init_status->host.sdr12  			= false;
init_status->host.sdr25 			= false;
init_status->host.sdr104 			= false;
init_status->host.hs		 		= false;

//Init card status
init_status->card.voltage_switch 	= false;
init_status->card.sdhc_sdxc  		= false;
init_status->card.rca  				= 0x0000;
init_status->card.cid.insert(0x0000000000000000,0x0000000000000000);
init_status->card.mem_ocr.reg_val  	= 0x00000000;
init_status->card.sdio_ocr.reg_val 	= 0x00000000;

//Init Result status
init_status->result.voltage_switch	= false;
init_status->result.sdio_init 		= false;
init_status->result.mem_init 		= false;
init_status->result.card_unusable	= false;
}
/*************************************************/





//SDIO,COMBO,MEM,VOLTAGE_SWITCH,
bool _host::card_init(uint32_t device, uint32_t slot,bool voltage_switch_enabled, _init_status *init_status)
{
  //_command *pcmd = new _command;
  //uint32_t retries = 0x01;
  uint32_t arg = 0x00;
  bool flag_f8 = true,sdio = true,mem = true;//check_mp = true;
  uint32_t error = 0x00;
  uint8_t host_spec_version = 0xFF;
 
 /*Send GO IDLE Command (CMD0)*/
  if(!p_card->go_idle(&error))
  return false;

  init_data_structure(init_status);
  p_card->reset_card_datastruct();
  
  /* Send Interface condition command (CMD 8) */
  _r7 if_cond;
  arg = ((0x01 << 8) | 0x000000AA);
  if(!p_card->send_if_cond(0x01,&if_cond,&error))
  {
    arg = ((0x02 << 8) | 0x000000AA);
	if(!p_card->send_if_cond(0x02,&if_cond,&error))	
		{
			if((error & 0x00000001) == 0x00000001)
				flag_f8 = false;/*Card Not responding to CMD8*/
		}
  }

  if((arg != if_cond.reg_val) && (flag_f8 == true))
  {
 	init_status->result.card_unusable = true; 
	return false;
  }

 init_status->result.card_unusable = false; 

 /*==================================================================================*/
  /*============================== SDIO Initialization ===============================*/
  /*==================================================================================*/
  /*Send Operating condition command (CMD5) enquiry CMD5 */
    _r4 sdio_ocr,r4;
	if(!p_card->get_sdio_ocr(&sdio_ocr,&error))
	{
		if((error & 0x00000001) == 0x00000001)/*No response*/
		{
			sdio 		= false;
			//check_mp 	= false;
		}
		else if(error > 0x00000001)/*Error Response*/
			sdio = false;
    }
	if(sdio)
	{
		if(!p_card->sdio_send_op_cond(true,&r4,&error))
		{
			sdio 		= false;
			//check_mp 	= false;
			host_logmsg("SDIO Card inside if : SDIO_OCR : %X\n",r4.reg_val);
  			HOST_LOG_DEBUG() << "SDIO Card inside if :SDIO_OCR" << std::hex << r4.reg_val << std::endl;
		}
		else
		{
			//host_logmsg("SDIO Card : SDIO_OCR : %X\n",sdio_ocr.reg_val);
  			HOST_LOG_DEBUG() << "SDIO Card :SDIO_OCR" << std::hex << sdio_ocr.reg_val << std::endl;
			//host_logmsg("SDIO Card inside if : SDIO_OCR : %X\n",r4.reg_val);
  			HOST_LOG_DEBUG() << "SDIO Card inside if  :SDIO_OCR" << std::hex << r4.reg_val << std::endl;
			sdio 		= true;	
			init_status->card.sdio_ocr.reg_val	= sdio_ocr.reg_val;
			if(r4.bit.mem_present == 0)
			mem			= false;
		}
		
		if(sdio)
		{
			 /* Get RCA */
			uint16_t rca ;
			if(!p_card->get_rca(&rca,&error))
			return false;
			//printf("rca = %x\n",rca);
  			HOST_LOG_DEBUG() << "Rca" << std::hex << rca << std::endl;
			init_status->card.rca = rca;
		}
	}

	_reg reg;
	reg32 capability;
	host_spec_version = get_spec_version(device, slot);
	init_status->host.version = host_spec_version;
	if(host_spec_version >= HOST_SPEC_VERSION_3_00 )
	{
		init_status->host.sdr12 = true;
		init_status->host.sdr25 = true;	
		init_status->host.voltage_switch = true; 
		/*Read CAP Register*/
		SET_SD_REGISTER_READ(&reg,HOST_CAPABILITIES_1,device,slot);
		if(!read_host_reg(&reg))
			return false;
		capability.reg_val = reg.sd_reg.value;  
		//fill the init status data

		init_status->host.sdr50 	= capability.bit.b0;  
		init_status->host.sdr104 	= capability.bit.b1;  
		init_status->host.ddr50 	= capability.bit.b2; 
		HOST_LOG_DEBUG() << "DDR50 is supported by Host controller\n" << std::endl;
  
		if((capability.bit.b0 == 0) && (capability.bit.b1 == 0) && (capability.bit.b2 == 0) )
			init_status->host.voltage_switch = false; 
	}

	/*Read CAP Register*/

  SET_SD_REGISTER_READ(&reg,HOST_CAPABILITIES,device,slot);
  if(!read_host_reg(&reg))
    return false;	
   capability.reg_val = reg.sd_reg.value; 	
   init_status->host.hs = capability.bit.b21;
  /*==================================================================================*/
  /*============================= MemCardInitialization ==============================*/
  /*==================================================================================*/

  if(mem) 
  {
  /* Send Operating condition command (ACMD41) enquiry ACMD41 */
    _r3 ocr;
	if(!p_card->get_ocr(&ocr,&error))
		return false;
	init_status->card.mem_ocr.reg_val = ocr.reg_val;
	/*Send Operation Condition -> s18r = 1, hcs = 1*/
	_r3 r3;	
	if(!p_card->send_op_cond(true,true,&r3,&error))
		return false;
	 
	

	if(r3.bit.ccs == 1)
	{
		//printf("SDHC or SDXC Card\n");
  	 	HOST_LOG_DEBUG() << "SDHC or SDXC Card " << std::endl;
		init_status->card.sdhc_sdxc = true;
	}
	else
	{
		//printf("SDSC Card\n");
  	 	HOST_LOG_DEBUG() << "SDHC Card " << std::endl;
		init_status->card.sdhc_sdxc = false;
	}

	//HOST_LOG_DEBUG() << "SDIO Card inside if  :SDIO_OCR" << std::hex << r4.reg_val << std::endl;
	HOST_LOG_DEBUG() << "R3 CCS value " <<std::hex << r3.reg_val <<std::endl;


//	HOST_LOG_DEBUG() << "R3 OCR value " << r3.bit.ocr <<std::endl;
	//HOST_LOG_DEBUG() << "R3 s18a value " <<std::hex << r3.reg_val <<std::endl;

	if(r3.bit.s18a == 1)
	{
		//printf("rdy to switch to 1.8V\n");
  	 	HOST_LOG_DEBUG() << "Ready to switch to 1.8V " << std::endl;
		init_status->card.voltage_switch = true;
		if(voltage_switch_enabled == true)
		{
  	 	HOST_LOG_DEBUG() << "Init_card with Voltage Switch enabled..and SPEC Version " <<std::hex << host_spec_version << std::endl;
		init_status->card.voltage_switch = true;
			if(host_spec_version >= HOST_SPEC_VERSION_3_00)
				init_status->result.voltage_switch = voltage_switch(device , slot);
		}		
	}
	else
		//printf("Continue current voltage\n");
  	 	HOST_LOG_DEBUG() << "Continue current Voltage " << std::endl;
		
	 /* Issue command ALL_SEND_CID (CMD2) to get the CID of the card */
  _r2 cid;
  if(!p_card->get_all_cid(&cid,&error))
	 return false;
  init_status->card.cid.insert(cid.get_rhs(),cid.get_lhs());	
  
   /* Get RCA */
  uint16_t rca = 0;
  _r6 rcaStatus;
  if(!p_card->get_rca_r6(&rca,&error,&rcaStatus))
     return false;
  init_status->card.rca = rca;
  
  if(rcaStatus.bit.app_cmd)
	HOST_LOG_DEBUG() << "APP_CMD is set....\n" << std::endl;
  
  if(0 == rca)
  {
  if(!p_card->get_rca(&rca,&error))
     return false;
  //printf("rca = %x\n",rca);
  HOST_LOG_DEBUG()<<"Rca" << std::hex << rca << std::endl;
  init_status->card.rca = rca;
}
 HOST_LOG_DEBUG()<<"rca = "<< std::hex  << rca<<std::endl;;

  /* Get Card Status */
 _r1 card_status;
  if(!p_card->get_card_status(&card_status,&error))
     return false;
 
  if(stby == card_status.bit.current_state)	 
  {
	 //printf("Current state : standby\n");
  	HOST_LOG_DEBUG() << "Current state " << std::endl;
  }
  else
	return false;
  }
  /*==================================================================================*/
  /*============================= Common Initialization ==============================*/
  /*==================================================================================*/ 

init_status->result.mem_init 	= mem;
init_status->result.sdio_init 	= sdio;
set_clock(device, slot , DS_MAX_FREQ , DIVIDED_CLK_SELECTION_MODE); 
return true;

}
/*************************************************/




/*************************************************/
/*************************************************/
bool _host::set_clock(uint32_t device, uint32_t slot, uint32_t Max_freq, uint32_t clock_selection_mode)
{
uint32_t base_clk = 0x00;
uint32_t clk_multiplier = 0x01;
uint8_t spec_version = get_spec_version( device, slot);
uint32_t freq_select,test_point = 0x02,actual_freq_select = 0x00;
_reg sd_register;
uint16_t clk_ctrl_reg = 0x00;
uint16_t loop_count = 0x0A;

SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES,device,slot);
read_host_reg(&sd_register);
base_clk = ((sd_register.sd_reg.value & 0x0000FF00) >> 8);


if(HOST_SPEC_VERSION_3_00 > spec_version)
{
	base_clk = base_clk & 0x03F;
}
base_clk = base_clk * 1000;
if(Max_freq == DEFAULT_FREQUENCY)
	Max_freq = 400;
else	
	Max_freq = Max_freq * 1000;

//printf("Max Freq  :%d\n",Max_freq);	
  			HOST_LOG_DEBUG() << "Max Freq:" << std::dec << Max_freq << std::endl;
//printf("Base Freq :%d\n",base_clk);
  			HOST_LOG_DEBUG() << "Base Freq:" << std::dec << base_clk << std::endl;


if(clock_selection_mode == PROGRAMABLE_CLK_SELECTION_MODE)
{
	if(HOST_SPEC_VERSION_3_00 == spec_version)
	{
		SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES_1,device,slot);
		read_host_reg(&sd_register);
		clk_multiplier = ((sd_register.sd_reg.value & 0x00FF0000) >> 16);
		if(0x00 == clk_multiplier)
		{
			//printf("PROGRAMABLE_CLK_SELECTION_MODE is not supported by the HeOST\n");
			HOST_LOG_DEBUG() << "PROGRAMABLE_CLK_SELECTION_MODE is not supported by the HeOST" << std::endl;
			return false;		
		}
		clk_multiplier++;
		//printf("Clock M = %d\n",clk_multiplier);
  		HOST_LOG_DEBUG() << "Clock M:" << std::dec << clk_multiplier << std::endl;
	}
	else
	{
		//printf("PROGRAMABLE_CLK_SELECTION_MODE is not supported by the HOST\n");
		HOST_LOG_DEBUG() <<"PROGRAMABLE_CLK_SELECTION_MODE is not supported by the HOST\n";
		return false;
	}
	freq_select 	= ((base_clk*clk_multiplier)/Max_freq);
	if(((base_clk*clk_multiplier)/freq_select) > Max_freq)
		freq_select++;

	actual_freq_select = freq_select - 1;
}

else
{
	if(base_clk < Max_freq)
	{
		actual_freq_select = 0x00;
	}
	else
	{
		freq_select 	= ((base_clk*clk_multiplier)/Max_freq);
		if(((base_clk*clk_multiplier)/freq_select) > Max_freq)
			freq_select++;
	
		if(HOST_SPEC_VERSION_3_00 <= spec_version)
		{
			if(freq_select == 1)
				actual_freq_select = 0x00;
			else
			{
				actual_freq_select = freq_select/2;
				if((actual_freq_select * 2) < freq_select)
					actual_freq_select++;
			}
		}
		else
		{
			if(freq_select == 1)
				actual_freq_select = 0x00;
			else
			{
				actual_freq_select = 0x01;
				while(test_point < freq_select)	
				{
					test_point = test_point << 1;
				}
				actual_freq_select = test_point/2;
			}				
		}
	}
}	
//printf("ferquency divider : %d\n",actual_freq_select);
	HOST_LOG_DEBUG() << "Frequency Divider:" << std::dec << actual_freq_select << std::endl;

clk_ctrl_reg	= ((uint16_t)((actual_freq_select & 0xFF) << 8) | (uint16_t)(spec_version >= HOST_SPEC_VERSION_3_00 ?((actual_freq_select & 0x300) >> 2) : 0x00 ) | ((uint16_t)(clk_multiplier > 1 ? (1<<5):0)) | ((uint16_t)(0x1)));

SET_SD_REGISTER_WRITE(&sd_register,HOST_CLOCK_CONTROL,device,slot,clk_ctrl_reg);
write_host_reg(&sd_register);
env::SleepMs(2);
read_host_reg(&sd_register);



while(!(sd_register.sd_reg.value & 0x0002))
{
	env::SleepMs(2);
	read_host_reg(&sd_register);
	if(!(loop_count--))
	return false;
}
//printf("Clock Ctrl Reg : %X\n", (clk_ctrl_reg | 0x0006));
	HOST_LOG_DEBUG() << "Clock Ctrl Reg:" << std::hex << clk_ctrl_reg << std::endl;

SET_SD_REGISTER_WRITE(&sd_register,HOST_CLOCK_CONTROL,device,slot,(clk_ctrl_reg | 0x0006));
write_host_reg(&sd_register);
return true;

}
/*************************************************/




/*************************************************/
bool _host::set_clk(uint16_t freq_select, uint32_t device, uint32_t slot)
{
  _reg clk_register;
  reg16 clkctrl;
  clkctrl.reg_val = (freq_select << 6);
  clkctrl.bit.b0  = 0;
  clkctrl.bit.b1  = 0; 
  clkctrl.bit.b2  = 0;

  SET_SD_REGISTER_WRITE(&clk_register,HOST_CLOCK_CONTROL,device,slot,clkctrl.reg_val);

  if(!write_host_reg(&clk_register))
    return false;
  return true;
 }
 /*************************************************/
 
 
 
 
 
 /*************************************************/
bool _host::card_clk(bool clk_state, uint32_t device,uint32_t slot)
{
  _reg clk_register;
  reg16 clkctrl;

  SET_SD_REGISTER_READ(&clk_register,HOST_CLOCK_CONTROL,device,slot);
  if(!read_host_reg(&clk_register))
    return false;  

  clkctrl.reg_val = clk_register.sd_reg.value;
  clkctrl.bit.b2 = (clk_state == true) ? 1 : 0;
  SET_SD_REGISTER_WRITE(&clk_register,HOST_CLOCK_CONTROL,device,slot,clkctrl.reg_val);

  if(!write_host_reg(&clk_register))
    return false;
	
  return true;
}
/*************************************************/



/*************************************************/
bool _host::host_clk(bool clk_state, uint32_t device,uint32_t slot)
{
  _reg clk_register;
  reg16 clkctrl;

  SET_SD_REGISTER_READ(&clk_register,HOST_CLOCK_CONTROL,device,slot);
  if(!read_host_reg(&clk_register))
    return false;  

  clkctrl.reg_val = clk_register.sd_reg.value;
  clkctrl.bit.b0 = (clk_state == true) ? 1 : 0;
  SET_SD_REGISTER_WRITE(&clk_register,HOST_CLOCK_CONTROL,device,slot,clkctrl.reg_val);

  if(!write_host_reg(&clk_register))
    return false;
	
  return true;
}
/*************************************************/






/*************************************************/
bool _host::dma_select(uint8_t value,uint32_t device, uint32_t slot)
{
  _reg hostctrl1_register;
  reg8 hostctrl;

  SET_SD_REGISTER_READ(&hostctrl1_register,HOST_HOST_CONTROL_1,device,slot);
  if(!read_host_reg(&hostctrl1_register))
    return false;  

  hostctrl.reg_val = hostctrl1_register.sd_reg.value;
  hostctrl.bit.b3 = (value & 0x01);
  hostctrl.bit.b4 = ((value & 0x02) >> 1);
  
  SET_SD_REGISTER_WRITE(&hostctrl1_register,HOST_HOST_CONTROL_1,device,slot,hostctrl.reg_val);
  //printf("select_dma : %X\n",hostctrl.reg_val );
	HOST_LOG_DEBUG() << "Select DMA:" << std::hex << hostctrl.reg_val << std::endl;
  if(!write_host_reg(&hostctrl1_register))
    return false;

return true;
}
/*************************************************/




/*************************************************/
bool _host::set_buswidth(bool buswidth, uint32_t device, uint32_t slot)
{
  _reg hostctrl1_register;
  reg8 hostctrl;

  SET_SD_REGISTER_READ(&hostctrl1_register,HOST_HOST_CONTROL_1,device,slot);
  if(!read_host_reg(&hostctrl1_register))
    return false;  

  hostctrl.reg_val = hostctrl1_register.sd_reg.value;
  hostctrl.bit.b1 = (buswidth == true) ? 1 : 0;

  SET_SD_REGISTER_WRITE(&hostctrl1_register,HOST_HOST_CONTROL_1,device,slot,hostctrl.reg_val);

  if(!write_host_reg(&hostctrl1_register))
    return false;

return true;
}
/*************************************************/




/*************************************************/
bool _host::set_highspeed(bool speed, uint32_t device, uint32_t slot)
{
  _reg hostctrl_reg;
  reg8 hostctrl;

 /*disable the SD clk before changing the speed */
  if(!host_clk(false,device,slot))
    return false;
  if(!card_clk(false,device,slot))
    return false; 
  
   /*Change speed*/
  SET_SD_REGISTER_READ(&hostctrl_reg,HOST_HOST_CONTROL_1,device,slot);
  if(!read_host_reg(&hostctrl_reg))
    return false;  

  hostctrl.reg_val = hostctrl_reg.sd_reg.value;
  hostctrl.bit.b2 = (speed == true) ? 1 : 0;
  SET_SD_REGISTER_WRITE(&hostctrl_reg,HOST_HOST_CONTROL_1,device,slot,hostctrl.reg_val);

  if(!write_host_reg(&hostctrl_reg))
    return false;
 /*disable the SD clk before changing the speed */
  if(!host_clk(true,device,slot))
    return false;
  if(!card_clk(true,device,slot))
    return false; 
	
return true;
}
/*************************************************/




/*************************************************/
bool _host::set_uhspeedmode(uint32_t device, uint32_t slot, uint8_t speed_mode)
{
  _reg hostctrl_reg;

 /*disable the SD clk before changing the speed */
 // if(!host_clk(false,device,slot))
 //   return false;
  if(!card_clk(false,device,slot))
    return false; 
  
   /*Change speed*/
  SET_SD_REGISTER_READ(&hostctrl_reg,HOST_HOST_CONTROL_2,device,slot);
  if(!read_host_reg(&hostctrl_reg))
    return false; 
	
   HOST_LOG_DEBUG() <<"UHS Mode selected : " <<((hostctrl_reg.sd_reg.value & 0xFFF8) | speed_mode) << std::endl;
	hostctrl_reg.sd_reg.value = ((hostctrl_reg.sd_reg.value & 0xFFF8) | speed_mode);
   if(!write_host_reg(&hostctrl_reg))
    return false;
  if(!read_host_reg(&hostctrl_reg))
    return false; 
//printf("UHS Mode selected : %X",(hostctrl_reg.sd_reg.value));	
  HOST_LOG_DEBUG() << "UHS Mode selected" << std::hex << hostctrl_reg.sd_reg.value << std::endl;
	/*disable the SD clk before changing the speed */
//  if(!host_clk(true,device,slot))
//    return false;
  if(!card_clk(true,device,slot))
    return false; 
	
return true;
}
/*************************************************/



/*************************************************/
bool _host::set_transfermode(bool mode, bool direction, uint8_t autocmd, bool block_count, bool dma, uint32_t device, uint32_t slot)
{
  _reg transfer_mode_reg;
  reg16 transfermode;

  SET_SD_REGISTER_READ(&transfer_mode_reg,HOST_TRANSFER_MODE,device,slot);
  if(!read_host_reg(&transfer_mode_reg))
    return false;  

  transfermode.reg_val = transfer_mode_reg.sd_reg.value;
  transfermode.bit.b0 = (dma == true) ? 1 : 0;
  transfermode.bit.b1 = (block_count == true) ? 1 : 0;
  transfermode.bit.b2 = (autocmd & 0x01);
  transfermode.bit.b3 = ((autocmd >> 1)& 0x01);
  transfermode.bit.b4 = (direction == true) ? 1 : 0;
  transfermode.bit.b5 = (mode == true) ? 1 : 0;

 // std::cout << "Transfer mode reg : "<< std::hex << transfermode.reg_val << std::endl;
  HOST_LOG_DEBUG() << "Transfer mode reg:" << std::hex << transfermode.reg_val <<std::endl;

  SET_SD_REGISTER_WRITE(&transfer_mode_reg,HOST_TRANSFER_MODE,device,slot,transfermode.reg_val);

  if(!write_host_reg(&transfer_mode_reg))
    return false;

return true;
}
/*************************************************/





/*************************************************/
bool _host::set_block_length_n_count(uint16_t length, uint16_t count,uint32_t device, uint32_t slot)
{
  _reg reg;
  reg16 block_size;

  /*Set Block Length*/
  SET_SD_REGISTER_READ(&reg,HOST_BLOCK_SIZE,device,slot);
  if(!read_host_reg(&reg))
    return false;  

  block_size.reg_val = (reg.sd_reg.value & 0xF000);
  block_size.reg_val = (block_size.reg_val | (length & 0x0FFF));
// printf("Block Length : %x\n",block_size.reg_val );
  SET_SD_REGISTER_WRITE(&reg,HOST_BLOCK_SIZE,device,slot,block_size.reg_val);
  if(!write_host_reg(&reg))
    return false;

  /*Set Block Count*/
  SET_SD_REGISTER_WRITE(&reg,HOST_BLOCK_COUNT,device,slot,count);
  if(!write_host_reg(&reg))
    return false;
	
return true;
}
/*************************************************/





/*************************************************/
bool _host::set_sdma_buffer_boundry(uint32_t device, uint32_t slot , uint32_t boundry)
{
  _reg reg;
  reg16 block_size;

  /*Set Block Length*/
  SET_SD_REGISTER_READ(&reg,HOST_BLOCK_SIZE,device,slot);
  if(!read_host_reg(&reg))
    return false;  

  block_size.reg_val = ((reg.sd_reg.value & 0x0FFF) | ((boundry<<12) & 0xF000));

  SET_SD_REGISTER_WRITE(&reg,HOST_BLOCK_SIZE,device,slot,block_size.reg_val);
  if(!write_host_reg(&reg))
    return false;
return true;
}
/*************************************************/






/*************************************************/
bool _host::set_timeout_ctrl(uint8_t timeout,uint32_t device, uint32_t slot)
{
  _reg reg;
  /*Set Block Count*/
  SET_SD_REGISTER_WRITE(&reg,HOST_TIMEOUT_CONTROL,device,slot,(timeout & 0x0F));
  if(!write_host_reg(&reg))
    return false;
  return true;
}
/*************************************************/



/*************************************************/
bool _host::voltage_switch(uint32_t device, uint32_t slot)
{
_reg reg;
reg32 present_state_reg, capability , host_ctrl2 ;
uint32_t error = 0x00;
_r1 status;

   /*Read CAP Register*/
  SET_SD_REGISTER_READ(&reg,HOST_CAPABILITIES_1,device,slot);
  if(!read_host_reg(&reg))
    return false;
  capability.reg_val = reg.sd_reg.value;  
  //fill the init status data
  
  if((capability.bit.b0 == 0) && (capability.bit.b1 == 0) && (capability.bit.b2 == 0) )
  {
//	printf("Host not capable of doing voltage switch\n");
       HOST_LOG_DEBUG()<<"Host not capable of doing voltage switch" << std::endl;
    return false; 
  }
  
 /*Send Voltage Switch Command (CMD11)*/
  if(!p_card->voltage_switch(&status,&error))
	return false;
	/*Disable SD clk*/
  if(!card_clk(false,device,slot))
    return false; 	
  env::SleepMs(5);	
  /*Read Present State Register*/
  SET_SD_REGISTER_READ(&reg,HOST_PRESENT_STATE,device,slot);
  if(!read_host_reg(&reg))
    return false;
  present_state_reg.reg_val = reg.sd_reg.value;
  if((present_state_reg.bit.b24))
  {
    HOST_LOG_DEBUG() << "CMD line is not driven low by the card" << std::endl;
    return false; 
  }	
  if((present_state_reg.bit.b20 != 0) || (present_state_reg.bit.b21 != 0) || (present_state_reg.bit.b22 != 0) || (present_state_reg.bit.b23 != 0))
  {
	HOST_LOG_DEBUG() <<"DAT lines are not driven low by the card" << std::endl;
    return false; 
  }

  /*Set 1.8V signaling in host ctrl2 register*/
  SET_SD_REGISTER_READ(&reg,HOST_HOST_CONTROL_2,device,slot);
  if(!read_host_reg(&reg))
    return false;
  host_ctrl2.reg_val = reg.sd_reg.value;  
  host_ctrl2.bit.b3 = 0x01;
  SET_SD_REGISTER_WRITE(&reg,HOST_HOST_CONTROL_2,device,slot,host_ctrl2.reg_val);
  if(!write_host_reg(&reg))
    return false ; 
  env::SleepMs(10);
  if(!read_host_reg(&reg))
    return false;
  if(!(host_ctrl2.bit.b3))	
    return false;
  /*Enable SD clk*/
  if(!card_clk(true,device,slot))
    return false; 
  env::SleepMs(5);	

     /*Read Present State Register*/
  SET_SD_REGISTER_READ(&reg,HOST_PRESENT_STATE,device,slot);
  if(!read_host_reg(&reg))
    return false;
  present_state_reg.reg_val = reg.sd_reg.value;
  if(!(present_state_reg.bit.b24))
  {
	//printf("CMD line is not driven Higah by the card\n");
	 HOST_LOG_DEBUG() << "CMD line is not driven Higah by the card\n" << std::endl;
    return false; 
  }	
  if((present_state_reg.bit.b20 == 0) || (present_state_reg.bit.b21 == 0) || (present_state_reg.bit.b22 == 0) || (present_state_reg.bit.b23 == 0))
  {
	HOST_LOG_DEBUG() << "DAT lines are not driven High by the card" << endl;
    return false; 
  }
 return true;
}
/*************************************************/




/*************************************************/
bool _host::get_sd_status(uint32_t device , uint32_t slot , bool speed , bool width, _r1 *status , _sd_status *sd_status)
{
	//uint64_t data = 0x00;
	uint8_t buffer[0x40];
   // uint32_t loop_c1 =0x00;
	uint32_t error = 0x00;	
	
	if(!select_card(status,&error))
		return false;

	if(!init_data_transfer(device , slot ,speed, width, 0x00 , 0x40))
		return false;
	
	if(!set_transfermode(false, true, 0x00, false, false, 0,0))
		return false;
	
	if(!did_host->p_card->get_sd_status(status,&error))
		return false;
		
	if(!read_buffer(device , slot,0x40, 0x00,&buffer[0]))
	return false;	

	env::SleepMs(150);
    if(!transfer_complete( &error, 0x10))
	{
		HOST_LOG_DEBUG() << "TRNS complete error : " << std::hex << error;
		return false;
	}
    
	sd_status->data_bus_width 				= ((buffer[0] >> 6)& 0x03);
	sd_status->secured_mode   				= (buffer[0] & 0x20) > 0 ? true : false;
	sd_status->security_funtions 			= (((buffer[0]) & 0x1F) | ((buffer[1]) & 0xE0));	
	sd_status->sd_card_type 				= (uint16_t)(((uint16_t)(buffer[2] << 8)) | ((uint16_t)(buffer[3])));	
	sd_status->size_of_protected_area 		= (uint32_t)(((uint32_t)(buffer[4] << 24)) | ((uint32_t)(buffer[5]) << 16) | ((uint32_t)(buffer[6]) << 8) | ((uint32_t)(buffer[7])));	
	sd_status->speed_class 					= buffer[8];
	sd_status->performance_move 			= buffer[9];	
	sd_status->au_size 						= (buffer[10] >> 4);	
	sd_status->erase_size 					= (uint16_t)((uint16_t)(buffer[11] << 8) | (uint16_t)(buffer[12]) );
	sd_status->erase_timeout 				= ((buffer[13] >> 2));
	sd_status->erase_offset 				= ((buffer[13] & 0x03));
	sd_status->uhs_speed_grade 				= ((buffer[14] >> 4));
	sd_status->uhs_au_size 					= ((buffer[14] & 0x0F));
	
	if(!deselect_card(status ,&error))
		return false;
	return true;
}
/*************************************************/





/*************************************************/
bool _host::int_status_enable(uint32_t device, uint32_t slot , uint32_t int_status)
{
  _reg reg;

  SET_SD_REGISTER_WRITE(&reg,HOST_INT_ENABLE,device,slot,(uint32_t)int_status);
  if(!write_host_reg(&reg))
    return false;
  return true;
}
/*************************************************/





/*************************************************/
bool _host::int_signal_enable(uint32_t device, uint32_t slot , uint32_t int_status)
{
  _reg reg;

  SET_SD_REGISTER_WRITE(&reg,HOST_SIGNAL_ENABLE,device,slot,(uint32_t)int_status);
  if(!write_host_reg(&reg))
    return false;
  return true;
}
/***********************************************************/




/***********************************************************/
bool _host::force_event(uint32_t device , uint32_t slot, uint32_t force)
{
  _reg reg;

  /* Set INT status enable register */
  SET_SD_REGISTER_WRITE(&reg,HOST_SET_ACMD_ERROR,device,slot,(uint32_t)force);
  reg.sd_reg.width = 0x04;
  if(!write_host_reg(&reg))
    return false;
  return true;
}
/***********************************************************/




/***********************************************************/
uint32_t _host::get_int_status(uint32_t device , uint32_t slot)
{
  _reg reg;
  SET_SD_REGISTER_READ(&reg,HOST_INT_STATUS,device,slot);
  read_host_reg(&reg);

  SET_SD_REGISTER_WRITE(&reg,HOST_INT_STATUS,device,slot,reg.sd_reg.value);
  write_host_reg(&reg);

  return ((uint32_t)reg.sd_reg.value);
}

uint16_t _host::get_autocmd_error(uint32_t device , uint32_t slot)
{
  _reg reg;
  SET_SD_REGISTER_READ(&reg,HOST_ACMD_ERR,device,slot);
  read_host_reg(&reg);

  return ((uint16_t)reg.sd_reg.value);
}
uint8_t _host::get_adma_error(uint32_t device , uint32_t slot)
{
  _reg reg;
  SET_SD_REGISTER_READ(&reg,HOST_ADMA_ERROR,device,slot);
  read_host_reg(&reg);

  return ((uint8_t)reg.sd_reg.value);
}

bool _host::init_data_transfer(uint32_t device , uint32_t slot , bool speed, bool bus_width,uint16_t blockcount , uint32_t block_length )
{
	_r1 status;
	uint8_t buswidth = 0x00;
	uint32_t error = 0x00;
	if(!set_timeout_ctrl(0x0E,0,0))
	return false;
	buswidth = (bus_width == true) ? 0x02 : 0x00;
	get_card_status(&status,&error);
	if(0x00 == status.bit.card_is_locked)	 
	{
		 HOST_LOG_DEBUG() << "set_card_buswidth: Card Locked" << std::endl;
		if(!set_card_buswidth(buswidth,&status,&error))
		return false;
	}
	if(!set_host_buswidth( bus_width, 0,0))
	return false;
	
	if(!set_speed(speed, 0, 0))
		return false;
	
	if(!set_card_blocklength(block_length,&status,&error))
		return false;

	
	//if(!set_card_blockcount(blockcount,&status))
	//return false;
	
	if(!set_host_block_length_n_count( block_length,  blockcount, device,slot))
	return false;

	return true;
}

bool _host::transfer_complete( uint32_t *error, uint32_t retry=0x10)
{
	return(did_host->p_card->trns_complete(error, retry));
}
bool _host::command_complete( uint32_t *error)
{
return(did_host->p_card->cmd_complete(error));
}
bool _host::dma_complete( uint32_t *error)
{
return(did_host->p_card->dma_complete(error));
}
bool _host::block_gap( uint32_t *error)
{
return(did_host->p_card->block_gap(error));
}


bool write_buffer(uint32_t device,uint32_t slot,uint32_t blk_size , uint32_t blk_count , uint8_t *ptr_buffer)
{
	_reg sd_register;
	reg32 present_state;
	uint32_t reg = HOST_BUFFER_0;
	uint32_t loop_c1 =0x00;
	uint32_t block = 0x00;
	
	do{
		//env::SleepMs(50);
		SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,device,slot);

		env::SleepMs(100);
		if(!read_host_reg(&sd_register))
			return false;
		present_state.reg_val = sd_register.sd_reg.value;

		if(present_state.bit.b10 == 0x01)
		{
			HOST_LOG_DEBUG() << "Host  ready to transfer data" << std::endl;
		for (loop_c1 = 0; loop_c1 < blk_size; loop_c1++)
			{
				SET_SD_REGISTER_WRITE(&sd_register,reg,device,slot,ptr_buffer[loop_c1 + (blk_size * block)]);
				//printf("write_buffer: Host ready to transfer data: data = %x\n", ptr_buffer[loop_c1 + (blk_size * block)]);

	HOST_LOG_DEBUG() << "wtite_buffer: Host ready transfger data:data" << std::hex << ptr_buffer[loop_c1 + (blk_size * block)]<< std::endl;
				if(!write_host_reg(&sd_register))
				return false;
			
				if(++reg > HOST_BUFFER_3)
					reg = HOST_BUFFER_0;
			}
		}
		else
		{
			HOST_LOG_DEBUG() << "Host Not ready to transfer data" << endl;
			return false;
		}
	
		block++;
	}while(block < blk_count);
	return true;
}

bool read_buffer(uint32_t device, uint32_t slot , uint32_t blk_size,uint32_t blk_count, uint8_t *ptr_buffer)
{
	_reg sd_register;
	reg32 present_state;
	uint32_t reg = HOST_BUFFER_0;
    uint32_t loop_c =0x00;
	uint32_t block = 0x00;
	
	SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,device,slot);
	if(!read_host_reg(&sd_register))
		return false;
 	present_state.reg_val = sd_register.sd_reg.value;
	do
	{
		env::SleepMs(10);
		SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,device,slot);
		if(!read_host_reg(&sd_register))
			return false;
		present_state.reg_val = sd_register.sd_reg.value;
	
		if(present_state.bit.b11 == 0x01)
		{
			 HOST_LOG_DEBUG () << "Host ready to transfer data" << std::endl;
 
			for (loop_c = 0; loop_c < blk_size; loop_c++)
			{
				SET_SD_REGISTER_READ(&sd_register,reg,device,slot);
				if(!read_host_reg(&sd_register))
					return false;
				
				ptr_buffer[loop_c + (block * blk_size)]	= (uint8_t)(sd_register.sd_reg.value & 0x00000000000000FF);
				//printf("read_buffer: Host ready to transfer data: data = %x\n", sd_register.sd_reg.value);
				HOST_LOG_DEBUG() << "read_buffer: Host ready to transfer data: data " << sd_register.sd_reg.value;
 
				if(++reg > HOST_BUFFER_3)
					reg = HOST_BUFFER_0;
			}
		}
		
		else
		{
			//printf("Host not ready for data transfer");
			HOST_LOG_DEBUG() << "Host not ready for data transfer" << std::endl;
			return false;
		}

		block++;
	}while(blk_count > block);
	return true;
}
/*********************************************************************************/


/********************************************************************************/
#if SD_PCI
void _host::get_bus_device_function(uint32_t device , uint16_t *bus_number , uint16_t *device_number , uint16_t *function_number)
{
	p_pci_handler->get_bus_device_function( device , bus_number , device_number ,  function_number);
}
#endif
/*********************************************************************************/


bool _host::pio_write(uint32_t device, uint32_t slot,bool speed, bool bus_width,uint8_t autocmd , uint32_t card_address,uint8_t *ptr_buffer,uint32_t blk_size,uint32_t blk_count,bool block_gap_enable)
{
	_r1 status;
	uint32_t error = 0x00;
	uint32_t block_trnsfd = 0x00;
	//uint32_t irq_status = 0x00;
	_reg sd_register;
	HOST_LOG_DEBUG() << "-------------------- PIO Write Start -----------------" << std::endl;
	if(!select_card(&status,&error))
	return false;
	
	if(get_card_status( &status, &error))
	{
		if(tran == status.bit.current_state)
		 HOST_LOG_DEBUG()<<"Card is in Trans state" <<std::endl	;	
	}

	
	
	if(!init_data_transfer(device , slot ,speed, bus_width, blk_count , blk_size ))
	return false;	
	
	
	
	if(!set_transfermode((blk_count == 0 ? false : true), false, (block_gap_enable == false ? autocmd : 0x00), (blk_count == 0 ? false : true), false, device,slot))
	return false;
	
	// do not select DMA, its an hack to fix DDR50 PIO transfer
	dma_select(0x01,device, slot); 

	if(autocmd == 0x02)
	{
		_reg reg;
		SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,blk_count);
		write_host_reg(&reg);
	}
	
	if(blk_count == 0x00)
	{
		if(!write_singleblock(card_address,&status,&error))
			return false;	
	}
	else
	{
		if(!write_multiblock(card_address,&status,&error))
			return false;
	}
	env::SleepMs(100);
	
	if((block_gap_enable == true) && (blk_count != 0x00))
	{
		HOST_LOG_DEBUG () <<"Setting the block Gap" << std::endl;
		for(block_trnsfd = 0x01 ; block_trnsfd <= blk_count ; block_trnsfd++)
		{
			SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
			if(!read_host_reg(&sd_register))
				return false;
			sd_register.sd_reg.value |= 0x01;
			if(!write_buffer(device, slot,blk_size , 1, &ptr_buffer[(block_trnsfd - 1) * blk_size]))
				return false;
			if(!write_host_reg(&sd_register))
				return false;
			env::SleepMs(20);

			if(block_trnsfd != blk_count)
			{
				sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFE) | 0x02;
				if(block_gap( &error))
				{	
					if(!transfer_complete( &error))
					{
						HOST_LOG_DEBUG() << "TRNS complete error : " << std::hex << error << std::endl ;
 
						return false;
					}
					//irq_status = get_int_status( device ,  slot);
				}
				else
					return false;

			}
			else
				sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFD);
			if(!write_host_reg(&sd_register))
				return false;
		}

	}
	else
	{
		if(!write_buffer(device, slot,blk_size , blk_count, ptr_buffer))
			return false;
	}
	
	env::SleepMs(100);
	
    if(!transfer_complete( &error, 0x10))
	{
		//printf("TRNS complete error : %X\n",error);
		//CORE_LOG_DEBUG (logger()) << "TRNS complete error : " << std::hex <<error;
		HOST_LOG_DEBUG () << "TRNS complete error : " << std::hex <<error;
		return false;
	}

	if((block_gap_enable == true) && (blk_count != 0x00))	
	{	
		if(!asynchronous_abort(device,slot,&status,&error))
			return false;
	}
	

	if(!get_card_status( &status, &error))
	{
		printf("Failed in get_card_status\n");
		return false;
	}
	//if(!init_data_transfer(device , slot ,speed, bus_width, 0x00 , 0x04))
	//	return false;
	//if(!set_card_blocklength(0x04,&status,&error))
	//return false;
	
	if(!set_host_block_length_n_count( 0x04,  0x00, device,slot))
	return false;
		
    if(!set_transfermode(false, true, 0x00, false, false, 0,0))
	   return false;
	
	
    send_num_wr_blocks(&status, &error);//CMD55 and ACMD22
	uint8_t buffer[0x40];
	if(!read_buffer(device , slot,0x04, 0x00,&buffer[0]))


	return false;
	
	for (int i = 0 ; i < 4 ; i++)
	{
		printf (" ACMD22 [%d] = %02X \n",i,buffer[i]);
	}
	
	env::SleepMs(150);

	if(!transfer_complete( &error, 0x10))
	{
		printf("TRNS complete error : %x\n",error);
		return false;
	}
	
/*	
	if(!get_card_status( &status, &error))
	{
		printf("Failed in get_card_status\n");
		return false;
	}
	
*/
	if(!deselect_card( &status,&error))
	return false;

	if(get_card_status( &status, &error))

	{
		if(stby == status.bit.current_state)
		//CORE_LOG_DEBUG (logger()) << "Card is in stby state" << std::endl;
		HOST_LOG_DEBUG () << "Card is in stby state" << std::endl;
		else
		{
			//CORE_LOG_DEBUG(logger ()) << "Card not in stby state" << std::endl	;
			HOST_LOG_DEBUG() << "Card not in stby state" << std::endl	;
			return false;
		}	
	}
	//CORE_LOG_DEBUG(logger()) << "-------------------- PIO Write End   -----------------" << std:endl;	
	HOST_LOG_DEBUG()<< "-------------------- PIO Write End   -----------------" << std::endl;	
	return true;
}

bool _host::pio_read(uint32_t device, uint32_t slot,bool speed, bool bus_width,uint8_t autocmd , uint32_t card_address,uint8_t *ptr_buffer,uint32_t blk_size,uint32_t blk_count,bool block_gap_enable)
{
	_r1 status;
	uint32_t error = 0x00;
	uint32_t block_trnsfd 	= 0x00;
	//uint32_t irq_status 	= 0x00;
	_reg sd_register;
	 //CORE_LOG_DEBUG (logger()) << "-------------------- PIO Read Start  -----------------" << std::endl;
	 HOST_LOG_DEBUG () << "-------------------- PIO Read Start  -----------------" << std::endl;
	if(!select_card(&status,&error))
	return false;
		
	if(get_card_status( &status,&error))
	{
		if(tran == status.bit.current_state)
		HOST_LOG_DEBUG () << "Card is in Trans state" << std::endl	;	
		//CORE_LOG_DEBUG (logger()) << "Card is in Trans state" << std::endl	;	
		//Log.Debug << "Current state : tran"  <<std::endl;
	}

	
	
	if(!init_data_transfer(device , slot ,speed, bus_width, blk_count, blk_size ))
	return false;
	
	if(!set_transfermode((blk_count == 0x00 ? false : true), true,(block_gap_enable == false ? autocmd : 0x00), (blk_count == 0x00 ? false : true), false, device,slot))
		return false;

	// do not select DMA, its an hack to fix DDR50 PIO transfer
	dma_select(0x01,device, slot);


	if(autocmd == 0x02)
	{
		_reg reg;
		SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,blk_count);
		write_host_reg(&reg);
	}

	if(blk_count == 0x00)
	{
		if(!read_singleblock( card_address,&status,&error))
			return false;	
	}
	else
	{
		if(!read_multiblock(card_address,&status,&error))
			return false;
		if(block_gap_enable == true)	
		{
			SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
			if(!read_host_reg(&sd_register))
				return false;
			sd_register.sd_reg.value |= 0x01;
			if(!write_host_reg(&sd_register))
				return false;
		}	
	}


	env::SleepMs(50);
	bool readtwise = false;
	bool done = false;
	uint32_t count_read_buffer = 0x00;
	if((block_gap_enable == true) && (blk_count != 0x00))
	{	
	//CORE_LOG_DEBUG(logger()) <<"blk_count : " << std::dec << blk_count;
	HOST_LOG_DEBUG() <<"blk_count : " << std::dec << blk_count;
		for(block_trnsfd = 0x01 ; block_trnsfd <= blk_count ; block_trnsfd++)
		{
			//env::SleepMs(10);
			env::SleepMs(1000);
		#if 1
			if(readtwise == true)
			{
				readtwise = false;
				if(!read_buffer(device, slot,blk_size , 2, &ptr_buffer[(block_trnsfd - 1) * blk_size]))
					return false;
			}
			else
			{
				if(!read_buffer(device, slot,blk_size , 1, &ptr_buffer[(block_trnsfd - 1) * blk_size]))
				{
				}
				//return false;
			}
			
		#endif		
			if(block_trnsfd != blk_count)
			{

				sd_register.sd_reg.value = ((sd_register.sd_reg.value & 0xFE) | 0x02);
				if(block_gap( &error))
				{	
					do{
						if(!transfer_complete( &error))
						{
							if(++count_read_buffer < 20)
							{
								if(!read_buffer(device, slot,blk_size , 100, &ptr_buffer[(block_trnsfd - 1) * blk_size]))
								{
								//return false;						
								}
							}
							else
							{
								//CORE_LOG_DEBUG( logger ())<<("TRNS complete error : " <<std::hex <<error << std::endl;
								HOST_LOG_DEBUG()<<"TRNS complete error : " <<std::hex <<error << std::endl;
									return false;
							}		
						}
						else
							done = true;
					}while(!done); 
					//CORE_LOG_DEBUG(logger()) << "count_read_buffer "  << std::dec <<,count_read_buffer << std::endl;
					HOST_LOG_DEBUG() << "count_read_buffer "  << std::dec <<count_read_buffer << std::endl;
					//irq_status = get_int_status( device ,  slot);
				}
				else
					return false;

			}
			else
				sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFD);
			
			//CORE_LOG_DEBUG (logger()) <<"sd_register.sd_reg.value for continuing the tranfr = " <<std::dec <<sd_register.sd_reg.value;
			HOST_LOG_DEBUG () <<"sd_register.sd_reg.value for continuing the tranfr = " <<std::dec <<sd_register.sd_reg.value;
			if(!write_host_reg(&sd_register))
				return false;				

			//env::SleepMs(1);
				
			if(block_trnsfd != blk_count)
			{ 
				if(!read_host_reg(&sd_register))
					return false;
				
				sd_register.sd_reg.value |= 0x01;				
				//CORE_LOG_DEBUG (logger())<<"sd_register.sd_reg.value =" <<std::dec << sd_register.sd_reg.value;
				HOST_LOG_DEBUG()<<"sd_register.sd_reg.value =" <<std::dec << sd_register.sd_reg.value;
				if(!write_host_reg(&sd_register))
					return false;
			}
			env::SleepMs(500);
		}	
	}
	else
	{
		if(!read_buffer(device, slot , blk_size,blk_count, ptr_buffer))
		return false;
	}
	
	env::SleepMs(100);
	
	if(!transfer_complete( &error , 0x10))
	{
		//CORE_LOG_DEBUG(logger()) << "TRNS complete error : " << std::hex <<,error <<std::endl;
		HOST_LOG_DEBUG() << "TRNS complete error : " << std::hex <<error <<std::endl;
		return false;
	}
	
	if((block_gap_enable == true) && (blk_count != 0x00))	
	{	
		if(!asynchronous_abort(device,slot,&status,&error))
			return false;
	}
	
	if(!get_card_status( &status, &error))
	{
		printf("Failed in get_card_status\n");
		return false;
	}
	if(status.bit.out_of_range)
		printf("ERROR: OUT_OF_RANGE error occured....\n");
	
	if(!deselect_card(&status,&error))
		return false;
	
	if(get_card_status( &status, &error))
	{
		if(stby == status.bit.current_state)
		HOST_LOG_DEBUG()<<"Card is in stby state" << std::endl;
		else
		return false;
	}
	HOST_LOG_DEBUG() << "-------------------- PIO Read End    -----------------" << std::endl;
	return true;
}

//bool config_transfer(bool bus_width, uint32_t timeout , bool speed , block_size, block_count )
bool _host::sdma_write(uint32_t device , uint32_t slot , uint32_t card_address , uint32_t *phy_address , uint32_t mem_size, uint32_t blk_size, uint32_t blk_count, bool speed, bool width, uint8_t autocmd)
{
  _reg reg;
  _r1 r1_status;
  bool select_next_buffer = false;
  bool exit_transfer = true;  
  uint32_t add_index = 0x00, error = 0x00;
  uint32_t mem_size_array[10] = {(1024*4),(1024*8),(1024*16),(1024*32),(1024*64),(1024*128),(1024*256),(1024*512),0,0};
  //uint32_t total_page = 0x00;
  //uint32_t retry = 0x10;
  uint64_t total_data_byte = (uint64_t)(((uint64_t)blk_size) * ((uint64_t)(blk_count == 0 ? 1 : blk_count)));
  uint32_t current_phy_address = 0x00;
  uint32_t physical_mem_split = 0x00;
  uint32_t byte_trnsfd_per_buffer 	= 0x00;
  uint32_t total_byte_transfd 		= 0x00;
  uint32_t delay = 200;
  if(blk_count > 3000)
	delay += (blk_count - 3000)/20;   
 HOST_LOG_DEBUG() << std::endl <<"\n-------------------- SDMA Write Start -----------------" << std::endl;  

#if 0  
  if(blk_count > 0)
  {
	total_page = ((blk_size * blk_count)/mem_size_array[mem_size]);
	if((blk_size * blk_count) > (total_page * mem_size_array[mem_size]))
	total_page++;
	printf("total Page = %X\n",total_page);
	printf("total block = %X\n",blk_count);
	if(blk_count > 3000)
	retry += (blk_count - 3000)/200;
  }	
#endif	
	set_sdma_buffer_boundry(device, slot ,mem_size);
	if(!select_card(&r1_status,&error))
		return false;
	
  	if(get_card_status( &r1_status,&error))
	{
		if(tran == r1_status.bit.current_state)
			HOST_LOG_DEBUG() << "Card is in Trans state"<<endl;	
	}
  
  	init_data_transfer(device , slot ,speed, width, blk_count , blk_size );
  
  	set_transfermode( (blk_count == 0x00 ? false : true),false, autocmd, (blk_count == 0x00 ? false : true), true, device,slot);

	dma_select(0x00,device, slot);
	 
	SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,phy_address[add_index++]);
	
	current_phy_address = reg.sd_reg.value;
	physical_mem_split  = current_phy_address;
	write_host_reg(&reg);
 #if 0 
  if(blk_count > 0)
  {
	 total_page--;
	 printf("total Page = %X\n",total_page);	
  }
#endif
    if(blk_count == 0x00)
	{
		if(!write_singleblock(card_address,&r1_status,&error))
		{
			HOST_LOG_DEBUG() << "write_singleblock failed...."<<endl;	
			return false;
		}
				
	}
	else
	{
		if(!write_multiblock(card_address,&r1_status,&error))
		{
			HOST_LOG_DEBUG() << "write_multiblock failed....."<<endl;	
			return false;
		}
			
	#if 0	
		if(block_gap_enable == true)
		{
			SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
			if(!read_host_reg(&sd_register))
				return false;
			sd_register.sd_reg.value |= 0x01;

			if(!write_host_reg(&sd_register))
				return false;	
		}
	#endif	
	}
#if 0	
 	SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
			if(!read_host_reg(&sd_register))
				return false;
			sd_register.sd_reg.value |= 0x01;
	if((block_gap_enable == true) && (blk_count != 0x00))
	{
		for(block_trnsfd = 0x01 ; block_trnsfd <= blk_count ; block_trnsfd++)
		{
			env::SleepMs(20);

			if(block_trnsfd != blk_count)
			{
				sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFE) | 0x02;
				if(!block_gap(device , slot , &error))
					return false;
			}
			else
				sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFD);
			
			if(!write_host_reg(&sd_register))
				return false;
		}

	}
#endif
uint32_t int_status = 0x00;
	HOST_LOG_DEBUG()<<"SDMA Buffer Address : "<<std::hex<<reg.sd_reg.value;
	HOST_LOG_DEBUG()<<"Total Byte requested "<<std::dec<<total_data_byte<<endl;	
	HOST_LOG_DEBUG()<<"Total Byte Trasfd    :      "<<endl;
  do{	
		env::SleepMs(delay);	
		uint32_t error;
		int_status = get_int_status(device , slot);
		if(int_status & 0x00000002) // Trans complete int 
		{
			SET_SD_REGISTER_READ(&reg,HOST_DMA_ADDRESS,device,slot);
			read_host_reg(&reg);
			byte_trnsfd_per_buffer += (reg.sd_reg.value - physical_mem_split);
			total_byte_transfd     += (reg.sd_reg.value - physical_mem_split);				
			HOST_LOG_DEBUG()<<"Transfer Complete"<<endl;	
			exit_transfer = false;
		}
		else if(int_status & 0x00000008)
		{
			SET_SD_REGISTER_READ(&reg,HOST_DMA_ADDRESS,device,slot);
			read_host_reg(&reg);
			byte_trnsfd_per_buffer += (reg.sd_reg.value - physical_mem_split);
			total_byte_transfd     += (reg.sd_reg.value - physical_mem_split);
			select_next_buffer = true;
			HOST_LOG_DEBUG()<<"DMA interrupt"<<endl;
			if(byte_trnsfd_per_buffer < MAX_DATA_SIZE_PER_BUFFER)
			{
				if(((current_phy_address+MAX_DATA_SIZE_PER_BUFFER) - (reg.sd_reg.value) ) >  mem_size_array[mem_size])
				{
					write_host_reg(&reg);
					physical_mem_split = reg.sd_reg.value;
					select_next_buffer = false;
				}	
			}
			if(select_next_buffer)
			{
				byte_trnsfd_per_buffer = 0x00;
				SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,phy_address[add_index++]);
				current_phy_address = reg.sd_reg.value;
				physical_mem_split  = current_phy_address;
				write_host_reg(&reg);
			}
			HOST_LOG_DEBUG()<<"SDMA Buffer Address : "<<std::hex<<reg.sd_reg.value<<endl;
		}
		else
		{
			deselect_card( &r1_status,&error);
			return false;
		}
		//get_int_status(device , slot);
		
		HOST_LOG_DEBUG()<< "total_byte_transfd"<<std::dec<<total_byte_transfd;
	}while(exit_transfer);	
	HOST_LOG_DEBUG()<<std::endl;
	HOST_LOG_DEBUG()<<"Transfer Complete"<<std::endl;
	if(!deselect_card( &r1_status,&error))
		return false;
	
	if(get_card_status( &r1_status, &error))
	{
		if(stby == r1_status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in stby state"<<std::endl;
	}
	HOST_LOG_DEBUG()<<"-------------------- SDMA Write End   -----------------"<<std::endl;

	return true;
}

bool _host::sdma_read(uint32_t device , uint32_t slot , uint32_t card_address , uint32_t *phy_address , uint32_t mem_size, uint32_t blk_size, uint32_t blk_count, bool speed, bool width, uint8_t autocmd , sdma_red_datatype *datastruct)
{
  _reg reg;
  _r1 r1_status;
  bool select_next_buffer = false;
  bool exit_transfer = true;
  uint32_t add_index = 0x00,error = 0x00;
  uint32_t mem_size_array[10] = {(1024*4),(1024*8),(1024*16),(1024*32),(1024*64),(1024*128),(1024*256),(1024*512),0,0};
  uint64_t total_data_byte = (uint64_t)(((uint64_t)blk_size) * ((uint64_t)(blk_count == 0 ? 1 : blk_count)));
  uint32_t current_phy_address = 0x00;
  uint32_t physical_mem_split = 0x00;
  uint32_t byte_trnsfd_per_buffer 	= 0x00;
  uint32_t total_byte_transfd 		= 0x00;
  uint32_t split_array_index = 0;
  HOST_LOG_DEBUG()<<"-------------------- SDMA Read Start  -----------------"<<endl;

  uint32_t delay = 100;
  if(blk_count > 3000)
	delay += (blk_count - 3000)/20;  
  set_sdma_buffer_boundry(device, slot ,mem_size);
	if(!select_card(&r1_status,&error))
		return false;
	
  	if(get_card_status( &r1_status,&error))
	{
		if(tran == r1_status.bit.current_state)
		   HOST_LOG_DEBUG()<<"Card is in Trans state"<<endl;	
		//Log.Debug << "Current state : tran"  <<std::endl;
	}
  
  	 init_data_transfer(device , slot ,speed, width, blk_count , blk_size );
  
  	 set_transfermode( (blk_count == 0x00 ? false : true),true, autocmd, (blk_count == 0x00 ? false : true), true, device,slot);

	 dma_select(0x00,device, slot);
	 
	SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,phy_address[add_index++]);
	current_phy_address = reg.sd_reg.value;
	physical_mem_split  = current_phy_address;	 
		
	write_host_reg(&reg);

	if(blk_count == 0x00)
	{
		if(!read_singleblock( card_address,&r1_status,&error))
		{
			HOST_LOG_DEBUG()<<"read_singleblock failed ......"<<endl;
			return false;
		}
				
	}
	else
	{
		if(!read_multiblock(card_address,&r1_status,&error))
		{
			HOST_LOG_DEBUG()<<"read_multiblock failed ......"<<endl;
			return false;
			
		}
			
		

	}
uint32_t int_status = 0x00;
	HOST_LOG_DEBUG()<<"SDMA Buffer Address : "<<std::hex<<reg.sd_reg.value<<std::endl;
	HOST_LOG_DEBUG()<<"Total Byte requested "<<std::dec<<total_data_byte<<endl;	
	HOST_LOG_DEBUG()<<"Total Byte Trasfd    :      "<<endl;
	do{	
		env::SleepMs(delay);	
		uint32_t error;
		int_status = get_int_status(device , slot);
		if(int_status & 0x00000002) // Trans complete int 
		{
			SET_SD_REGISTER_READ(&reg,HOST_DMA_ADDRESS,device,slot);
			read_host_reg(&reg);
			byte_trnsfd_per_buffer += (reg.sd_reg.value - physical_mem_split);
			total_byte_transfd     += (reg.sd_reg.value - physical_mem_split);				
			datastruct[split_array_index].address 		= physical_mem_split;
			datastruct[split_array_index++].total_byte 	= (reg.sd_reg.value - physical_mem_split);		
			datastruct[split_array_index].address 		= 0x00;
			datastruct[split_array_index].total_byte	= 0x00;
			HOST_LOG_DEBUG()<<"Transfer Complete"<<std::endl;	
			exit_transfer = false;
		}
		else if(int_status & 0x00000008)
		{
			SET_SD_REGISTER_READ(&reg,HOST_DMA_ADDRESS,device,slot);
			read_host_reg(&reg);
			byte_trnsfd_per_buffer += (reg.sd_reg.value - physical_mem_split);
			total_byte_transfd     += (reg.sd_reg.value - physical_mem_split);
			datastruct[split_array_index].address 		= physical_mem_split;
			datastruct[split_array_index++].total_byte 	= (reg.sd_reg.value - physical_mem_split);
			
			select_next_buffer = true;
			HOST_LOG_DEBUG()<<"DMA interrupt"<<endl;
			if(byte_trnsfd_per_buffer < MAX_DATA_SIZE_PER_BUFFER)
			{
				if(((current_phy_address+MAX_DATA_SIZE_PER_BUFFER) - (reg.sd_reg.value) ) >  mem_size_array[mem_size])
				{
					write_host_reg(&reg);
					physical_mem_split = reg.sd_reg.value;
					select_next_buffer = false;
				}	
			}
			if(select_next_buffer)
			{
				byte_trnsfd_per_buffer = 0x00;
				SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,phy_address[add_index++]);
				current_phy_address = reg.sd_reg.value;
				physical_mem_split  = current_phy_address;
				write_host_reg(&reg);
			}
			HOST_LOG_DEBUG()<<"SDMA Buffer Address : "<<std::hex<<reg.sd_reg.value<<endl;
		}
		else
		{
			deselect_card( &r1_status,&error);
			return false;
		}
		//get_int_status(device , slot);
		
		
		HOST_LOG_DEBUG()<<std::dec<<total_byte_transfd;
	}while(exit_transfer);	
		//printf("Total Byte requested : %x\n",total_data_byte);
		//printf("Total Byte Trasfd    : %x\n",total_byte_transfd);	
	HOST_LOG_DEBUG()<<endl<<"Transfer Complete"<<endl;
	if(!deselect_card( &r1_status,&error))
		return false;
	
	if(get_card_status( &r1_status, &error))
	{
		if(stby == r1_status.bit.current_state)
		HOST_LOG_DEBUG()<<"Card is in stby state"<<endl;	
	}
	HOST_LOG_DEBUG()<<"-------------------- SDMA Read End    -----------------"<<endl;	
	return true;
}
bool _host::dma2_write(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_sel , bool block_gap_enable, bool asynchronus_abort)
{
  _reg reg,sd_register;
  _r1 r1_status;
  //uint32_t add_index = 0x00;
  uint32_t error = 0x00;
  bool block_count_valid = false;
  if((0 < blk_count) && (blk_count < 65536))
  block_count_valid = true;
  uint32_t block_trnsfd;
  uint16_t block_gap_int = 0x00;
  uint32_t retry = 0x10;
  if(blk_count > 3000)
	retry += (blk_count - 3000)/200;
  HOST_LOG_DEBUG()<<"-------------------- ADMA Write Start -----------------"<<endl;
  HOST_LOG_DEBUG()<<"total block"<<std::hex<<blk_count;
  
	if(!select_card(&r1_status,&error))
	{
		HOST_LOG_DEBUG()<<"ADMA Error"<<std::hex<<get_adma_error(device,slot)<<endl;
		return false;
	}
  	if(get_card_status( &r1_status, &error))
	{
		if(tran == r1_status.bit.current_state)
		HOST_LOG_DEBUG()<<"Card is in Trans state"<<endl;	
		//Log.Debug << "Current state : tran"  <<std::endl;
	}
  
  	 init_data_transfer(device , slot ,speed, width, blk_count , blk_size );
  
  	 set_transfermode( (blk_count == 0x00 ? false : true),false, autocmd, block_count_valid, true, device,slot);

	 dma_select(dma_sel,device, slot);
	 
	 SET_SD_REGISTER_WRITE(&reg,HOST_ADMA_ADDRESS,device,slot,dma_table);
	 //printf("Base Table Address : %X",reg.sd_reg.value);
	 write_host_reg(&reg);
	
	if(autocmd == 0x02)
	{
		_reg reg;
		SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,blk_count);
		write_host_reg(&reg);
	}
	if(blk_count == 0x00)
	{
		if(!write_singleblock(card_address,&r1_status,&error))
			{
				HOST_LOG_DEBUG()<<"ADMA Error"<<std::hex<<get_adma_error(device,slot);
				return false;	
			}
	}
	else
	{
		SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
		read_host_reg(&sd_register);
		sd_register.sd_reg.value |= 0x01;
		if(!write_multiblock(card_address,&r1_status,&error))
		{
			HOST_LOG_DEBUG()<<"ADMA Error"<<std::hex<<get_adma_error(device,slot);
			return false;
		}
		if(block_gap_enable == true)
		{
			write_host_reg(&sd_register);
		}
	}

	if(asynchronus_abort == true)
	{
		//env::SleepUs(10);
		if(!p_card->stop_transmission(&r1_status,&error))
			return false;
		HOST_LOG_DEBUG()<<"INT_STATUS"<<std::dec<<get_int_status(device , slot)<<endl;
		SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_COUNT,device,slot);
		read_host_reg(&sd_register);
		HOST_LOG_DEBUG()<<"BLOCK COUNT VALUE"<<std::dec<<sd_register.sd_reg.value<<endl;
		if(!reset(SW_DAT_LINE_RESET, device, slot))
			return false;
		if(!reset(SW_CMD_LINE_RESET, device, slot))
			return false;		
		if(!deselect_card( &r1_status,&error))
			return false;
	
		if(get_card_status( &r1_status,&error))
		{
			if(stby == r1_status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in stby state"<<std::endl;
		}
		return true;
	}

	if((block_gap_enable == true) && (blk_count != 0x00))
	{
		for(block_trnsfd = 0x01 ; block_trnsfd <= blk_count ; block_trnsfd++)
		{
			//env::SleepMs(20);
			SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
			read_host_reg(&sd_register);
			
			if(block_gap( &error))
			{				
				if(!transfer_complete( &error))
				{
					 HOST_LOG_DEBUG()<<"TRNS complete error "<<std::hex<<error<<std::endl;
					return false;
				}
				else
					block_gap_int++;
			}
			else
			{
				break;
			}
			
			sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFE) | 0x02;
			write_host_reg(&sd_register);
			sd_register.sd_reg.value |= 0x01;
			write_host_reg(&sd_register);
			
		}
	}
	else
	{
		env::SleepMs(100);
		if(!transfer_complete( &error,retry))
		{
			 HOST_LOG_DEBUG()<<"TRNS complete error "<<std::hex<<error<<std::endl;
			deselect_card(&r1_status,&error);
			return false;
		}
		else
		{
			 HOST_LOG_DEBUG()<<"Transfer Complete"<<endl;
		}
		//get_int_status(device , slot);
		if(!deselect_card( &r1_status,&error))
			return false;
	
		if(get_card_status( &r1_status,&error))
		{
			if(stby == r1_status.bit.current_state)
				HOST_LOG_DEBUG()<<"Card is in stby state"<<std::endl;
			else
				HOST_LOG_DEBUG()<<"Card is in state"<<std::hex<<r1_status.reg_val<<std::endl;	
		}
		HOST_LOG_DEBUG()<<"-------------------- ADMA Write End   -----------------"<<std::endl;
		return true;
	}
	
	if((block_gap_enable == true) && (blk_count != 0x00))
	{	
		if(!asynchronous_abort(device,slot,&r1_status,&error))
			return false;
		if(block_gap_int > 0x02)
			return true;
	}

	return false;
}

bool _host::dma2_write_3_4_16(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_sel , bool block_gap_enable, bool asynchronus_abort,uint8_t* LockData)
{
  _reg reg,sd_register;
  _r1 r1_status;
  //uint32_t add_index = 0x00;
  uint32_t error = 0x00;
  bool block_count_valid = false;
  if( (0 < blk_count) && (blk_count < 65536) )
  block_count_valid = true;
  uint32_t block_trnsfd;
  uint16_t block_gap_int = 0x00;
  uint32_t retry = 0x10;
  if(blk_count > 3000)
	retry += (blk_count - 3000)/200;
  HOST_LOG_DEBUG()<< "\n-------------------- ADMA Write Start -----------------\n"<<std::endl;
  HOST_LOG_DEBUG()<< "dma2_write_3_4_16: total block = %X\n"<<std::hex<< blk_count <<std::endl;
  
  	if(get_card_status( &r1_status, &error))
	{
		if(tran == r1_status.bit.current_state)
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is in Trans state\n"<<std::endl	;	
		//Log.Debug << "Current state : tran"  <<std::endl;
	}
		if(!r1_status.bit.card_is_locked)
		{
			HOST_LOG_DEBUG()<< "\n"<<std::endl;
			//Lock with password
			LockData[0] = 0x5;
			LockData[1] = 0x2;
			LockData[2] = 'a';
			LockData[3] = 'b';
			if(!lock_unlock(device, slot, LockData, speed,  width, &r1_status))
			{
				HOST_LOG_DEBUG()<< "dma2_write_3_4_16: set passwd check lock_unlock function\n"<<std::endl;
				return false; 
			}
			error = 0x00;
			if(!get_card_status( &r1_status, &error ))
			{
				HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Failed in get_card_status function\n"<<std::endl;
				return false; 
			}
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card Lock status: 0x%x\n"<<std::hex <<  r1_status.bit.card_is_locked<<std::endl;
			if((r1_status.bit.lock_unlock_fail) || (!r1_status.bit.card_is_locked))
			{
				HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Failed to Set the password\n"<<std::endl;
				if(!r1_status.bit.card_is_locked)
				{
					HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is still unlocked\n"<<std::endl;
				}
			}
		}
		else
		{
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is locked\n"<<std::endl;
		}
	
	if(r1_status.bit.card_is_locked == 1)
	{
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card Locked so CMD42 invoked to Unlock the Card\n"<<std::endl;
			LockData[0] = 0x2;
		if(!lock_unlock(device, slot,LockData, speed,  width ,&r1_status))
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card Failed to Unlock\n"<<std::endl;
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card Lock status: 0x%x\n"<<std::hex<< r1_status.bit.card_is_locked <<std::endl;
	}  
	if(!select_card(&r1_status,&error))
	{
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: ADMA Error : %X"<<std::hex<<get_adma_error(device,slot)<<std::endl;
		return false;
	}
  	if(get_card_status(&r1_status, &error))
	{
		if(tran == r1_status.bit.current_state)
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is in Trans state\n"<<std::endl	;	
		//Log.Debug << "Current state : tran"  <<std::endl;
	}
  	 init_data_transfer(device , slot ,speed, width, blk_count , blk_size );
  
  	 set_transfermode( (blk_count == 0x00 ? false : true),false, autocmd, block_count_valid, true, device,slot);

	 dma_select(dma_sel,device, slot);
	 
	 SET_SD_REGISTER_WRITE(&reg,HOST_ADMA_ADDRESS,device,slot,dma_table);
	 //HOST_LOG_DEBUG()<< "Base Table Address : %X"<<std::hex<<reg.sd_reg.value<<std::endl;
	 write_host_reg(&reg);
	
	if(autocmd == 0x02)
	{
		_reg reg;
		SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,blk_count);
		write_host_reg(&reg);
	}
	if(blk_count == 0x00)
	{
		if(!write_singleblock(card_address,&r1_status,&error))
			{
				HOST_LOG_DEBUG()<< "dma2_write_3_4_16: ADMA Error : %X"<<std::hex<<get_adma_error(device,slot) <<std::endl;
				return false;	
			}
	}
	else
	{
		SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
		read_host_reg(&sd_register);
		sd_register.sd_reg.value |= 0x01;
		if(!write_multiblock(card_address,&r1_status,&error))
		{
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: ADMA Error : %X"<<std::hex<<get_adma_error(device,slot) <<std::endl;
			return false;
		}
		if(block_gap_enable == true)
		{
			write_host_reg(&sd_register);
		}
	}

	if(asynchronus_abort == true)
	{
		//env::SleepUs(10);
		if(!p_card->stop_transmission(&r1_status,&error))
			return false;
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: INT_STATUS = %d\n"<<get_int_status(device , slot)<<std::endl;
		SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_COUNT,device,slot);
		read_host_reg(&sd_register);
		HOST_LOG_DEBUG()<< "dma2_write_3_4_16: BLOCK COUNT VALUE = %d\n"<<sd_register.sd_reg.value<<std::endl;
		if(!reset(SW_DAT_LINE_RESET, device, slot))
			return false;
		if(!reset(SW_CMD_LINE_RESET, device, slot))
			return false;		
		if(!deselect_card( &r1_status,&error))
			return false;
	
		if(get_card_status( &r1_status,&error))
		{
			if(stby == r1_status.bit.current_state)
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is in stby state\n"<<std::endl	;	
		}
		return true;
	}

	if((block_gap_enable == true) && (blk_count != 0x00))
	{
		for(block_trnsfd = 0x01 ; block_trnsfd <= blk_count ; block_trnsfd++)
		{
			//env::SleepMs(20);
			SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
			read_host_reg(&sd_register);
			
			if(block_gap( &error))
			{				
				if(!transfer_complete( &error))
				{
					HOST_LOG_DEBUG()<< "dma2_write_3_4_16: TRNS complete error : %X\n"<<std::hex<< error<<std::endl;
					return false;
				}
				else
					block_gap_int++;
			}
			else
			{
				break;
			}
			
			sd_register.sd_reg.value = (sd_register.sd_reg.value & 0xFE) | 0x02;
			write_host_reg(&sd_register);
			sd_register.sd_reg.value |= 0x01;
			write_host_reg(&sd_register);
			
		}
	}
	else
	{
		env::SleepMs(100);
		if(!transfer_complete( &error,retry))
		{
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: TRNS complete error : %X\n"<<std::hex<<error <<std::endl;
			deselect_card( &r1_status,&error);
			return false;
		}
		else
		{
			HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Transfer Complete\n"<<std::endl;
		}
		//get_int_status(device , slot);
		if(!deselect_card( &r1_status,&error))
			return false;
	
		if(get_card_status( &r1_status,&error))
		{
			if(stby == r1_status.bit.current_state)
				HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is in stby state\n"<<std::endl	;	
			else
				HOST_LOG_DEBUG()<< "dma2_write_3_4_16: Card is in state : %X\n"<<r1_status.reg_val<<std::endl	;	
		}
HOST_LOG_DEBUG()<< "-------------------- ADMA Write End   -----------------\n\n"<<std::endl;
		return true;
	}
	
	if((block_gap_enable == true) && (blk_count != 0x00))
	{	
		if(!asynchronous_abort(device,slot,&r1_status,&error))
			return false;
		if(block_gap_int > 0x02)
			return true;
	}

	return true;
}

bool _host::dma2_read(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_sel )
{
  _reg reg;
  _r1 r1_status;
  //uint32_t add_index = 0x00;
  uint32_t error=0x00;
  bool block_count_valid = false;
  if((0 < blk_count) && (blk_count < 65536))
  block_count_valid = true;
  HOST_LOG_DEBUG()<<"-------------------- ADMA Read Start  -----------------"<<std::endl; 
  HOST_LOG_DEBUG()<<"total block ="<<std::hex<<blk_count<<std::endl;
  
	if(!select_card(&r1_status,&error))
	{
		HOST_LOG_DEBUG()<<"ADMA Error :"<<std::hex<<get_adma_error(device,slot)<<endl;
		return false;
	}
	
  	if(get_card_status( &r1_status, &error))
	{
		if(tran == r1_status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in Trans state"<<endl;	
		//Log.Debug << "Current state : tran"  <<std::endl;
	}
  
  	 init_data_transfer(device , slot ,speed, width, blk_count , blk_size );
  
  	 set_transfermode( (blk_count == 0x00 ? false : true),true, autocmd, block_count_valid, true, device,slot);

	 dma_select(dma_sel,device, slot);
	 
	 SET_SD_REGISTER_WRITE(&reg,HOST_ADMA_ADDRESS,device,slot,dma_table);
	 //printf("Base Table Address : %X",reg.sd_reg.value);
	 write_host_reg(&reg);

	if(autocmd == 0x02)
	{
		_reg reg;
		SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,blk_count);
		write_host_reg(&reg);
	}
	 
	if(blk_count == 0x00)
	{
		if(!read_singleblock( card_address,&r1_status,&error))
			{
				HOST_LOG_DEBUG()<<"ADMA Error :"<<std::hex<<get_adma_error(device,slot)<<endl;
				return false;	
			}	
	}
	else
	{
		if(!read_multiblock(card_address,&r1_status,&error))
			{
				HOST_LOG_DEBUG()<<"ADMA Error : "<<std::hex<<get_adma_error(device,slot)<<endl;
				return false;	
			}
	}


	env::SleepMs(1000);	

	if(!transfer_complete( &error))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error : "<<error<<std::endl;
		deselect_card( &r1_status,&error);
		return false;
	}
	else
	{
		HOST_LOG_DEBUG()<<"Transfer Complete"<<std::endl;
	}
	get_int_status(device , slot);
	
	if(!deselect_card( &r1_status,&error))
		return false;
	
	if(get_card_status( &r1_status,&error))
	{
		if(stby == r1_status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in stby state"<<std::endl;
	}
	HOST_LOG_DEBUG()<<"-------------------- ADMA Read End    -----------------"<<endl;	
	return true;
}





bool _host::erase_data(uint32_t start_address,uint32_t end_address,_r1 *status)
{
uint32_t error = 0x00;
	if(!select_card(status,&error))
		return false;

	if(!erase_start_add(start_address,status,&error))
		return false;
	
	if(!erase_end_add(end_address,status,&error))
		return false;
	
	if(!erase(status,&error))
		return false;
		
	if(!deselect_card( status,&error))
		return false;
	
	return true;
}
bool issue_lock_unlock_cmd(_r1 *status,uint32_t *error)
{
return(did_host->p_card->lock_unlock(status,error)); 
}

bool _host::lock_unlock(uint32_t device, uint32_t slot,uint8_t *lock_card_data,bool speed, bool bus_width ,_r1 *status)
{
	_reg sd_register;
	reg32 present_state;
	//uint32_t data = 0x00;
	uint32_t error = 0x00;
	//uint32_t temp_data = 0x00;
	uint8_t total_data_size = lock_card_data[1] + 0x02;
	uint32_t reg = HOST_BUFFER_0;
	if(0x08 == lock_card_data[0])
	{
		total_data_size = 0x01;
	}
	if(!select_card(status,&error))
		return false;

	if(!init_data_transfer(device , slot ,speed, bus_width, 0x00 , total_data_size))
		return false;
	
	if(!set_transfermode(false, false, 0x00, false, false, 0,0))
		return false;
	
	if(!issue_lock_unlock_cmd(status,&error))
	{
		
		return false;
	}
	HOST_LOG_DEBUG() <<"In issue_lock_unlock_cmd:status->lock_unlock_fail:  "<< status->bit.lock_unlock_fail<< "card_is_locked:" << status->bit.card_is_locked <<std::endl;
	HOST_LOG_DEBUG() <<"lock unlock result:  "<< status->reg_val << endl;
/*
	if(get_card_status( status,&error))
	{
		if(rcv == status->bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in Receive data state"<<endl	;	
		else
		{
			HOST_LOG_DEBUG()<<"Card not in Receive data state"<<endl;
			return false;			
		}

		if(status->bit.card_is_locked == 1)
		{
			HOST_LOG_DEBUG()<<"Card is locked state now \n"<<endl;
		}
		else
		{
			HOST_LOG_DEBUG()<<"Card is unlocked state now \n"<<endl;
		}
	}

	*/
	
	env::SleepMs(100);	
	SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,0,0);
	if(!read_host_reg(&sd_register))
		return false;
	present_state.reg_val = sd_register.sd_reg.value;

	if(present_state.bit.b10 == 0x01)
	{
        uint8_t loop_c1 =0x00;
		for (loop_c1 = 0; loop_c1 < total_data_size+2; loop_c1++)
		{
			

			SET_SD_REGISTER_WRITE(&sd_register,reg,0,0,lock_card_data[loop_c1]);
			//printf("sd_register[reg].sd_reg.width: %x \t lock_card_data[loop_c1]: %x\n", sd_register.sd_reg.width, lock_card_data[loop_c1]); 
			if(!write_host_reg(&sd_register))
			return false;
			
		    if(reg == HOST_BUFFER_3)
			   reg = HOST_BUFFER_0;
			else
				reg++;
		}
    }

    if(!transfer_complete( &error))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error :"<<std::hex<<error;
		if(get_card_status( status,&error))
		{
			if(rcv == status->bit.current_state)
				HOST_LOG_DEBUG()<<"After TRNS complete: Card is in Receive data state"<<endl	;	
			else
			{
				HOST_LOG_DEBUG()<<"After TRNS complete: Card not in Receive data state"<<endl;
				return false;			
			}

			if(status->bit.card_is_locked == 1)
			{
				HOST_LOG_DEBUG()<<"After TRNS complete: Card is locked state now \n"<<endl;
			}
			else
			{
				HOST_LOG_DEBUG()<<"After TRNS complete: Card is unlocked state now \n"<<endl;
			}
		}
		return false;
	}

	HOST_LOG_DEBUG()<<"TRNS Success :" <<endl;

	if(get_card_status( status,&error))
	{
		if(rcv == status->bit.current_state)
			HOST_LOG_DEBUG()<<"After TRNS complete: Card is in Receive data state"<<endl	;	
		else
		{
			HOST_LOG_DEBUG()<<"After TRNS complete: Card not in Receive data state"<<endl;
		}

		if(status->bit.card_is_locked == 1)
		{
			HOST_LOG_DEBUG()<<"After TRNS complete: Card is locked state now \n"<<endl;
		}
		else
		{
			HOST_LOG_DEBUG()<<"After TRNS complete: Card is unlocked state now \n"<<endl;
		}
	}
	



	if(!deselect_card( status,&error))
		return false;
	return true;
}

bool _host::get_scr(uint32_t device , uint32_t slot, _scr *ptr_scr,_r1 *status,bool speed, bool width)
{
	uint64_t data = 0x00;
	uint8_t buffer[0x08];
    uint32_t loop_c1 =0x00;
	uint32_t error;	
	
	if(!select_card(status,&error))
		return false;

	if(!init_data_transfer(device , slot ,speed, width, 0x00 , 0x08))
		return false;
	
	if(!set_transfermode(false, true, 0x00, false, false, 0,0))
		return false;
	
	if(!did_host->p_card->get_scr(status,&error))
		return false;
		
	if(!read_buffer(device , slot,0x08, 0x00,&buffer[0]))
	return false;	

	env::SleepMs(150);
    if(!transfer_complete( &error))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error : "<<std::hex<<error;
		return false;
	}

	for (loop_c1 = 0; loop_c1 < 8; loop_c1++)
		data = ((data << 8)| (buffer[loop_c1]));

	ptr_scr->insert(data);
	if(!deselect_card( status,&error))
		return false;
	return true;
}

bool _host::get_supported_functions(uint32_t device,uint32_t slot, bool speed,bool width,_function *func, _r1 *status)
{
	uint8_t buffer[0x40];	
	uint32_t argument  = 0x0000FFFF;
	//uint32_t loop = 0x00;
	if(!issue_switch_function( device , slot ,  speed, width, argument, status, &buffer[0] ))
		return false;
	
	extract_switch_function_data(func,&buffer[0]);
	return true;
}
void extract_switch_function_data(_function *func,uint8_t *buffer)
{
	func->maxCurrent = (((uint16_t)buffer[1]) | ((uint16_t)buffer[0] << 8));
	
	func->group1 = (((uint16_t)buffer[13]) | ((uint16_t)buffer[12] << 8));
	func->group2 = (((uint16_t)buffer[11]) | ((uint16_t)buffer[10] << 8));
	func->group3 = (((uint16_t)buffer[9])  | ((uint16_t)buffer[8]  << 8));
	func->group4 = (((uint16_t)buffer[7])  | ((uint16_t)buffer[6]  << 8));
	func->group5 = (((uint16_t)buffer[5])  | ((uint16_t)buffer[4]  << 8));
	func->group6 = (((uint16_t)buffer[3])  | ((uint16_t)buffer[2]  << 8));	
	
	func->current_grp6 = (((uint16_t)buffer[14] & 0xF0) >> 4);
	func->current_grp5 = ((uint16_t)buffer[14] & 0x0F);
	func->current_grp4 = (((uint16_t)buffer[15] & 0xF0) >> 4);
	func->current_grp3 = ((uint16_t)buffer[15] & 0x0F);
	func->current_grp2 = (((uint16_t)buffer[16] & 0xF0) >> 4);
	func->current_grp1 = ((uint16_t)buffer[16] & 0x0F);
	
	func->struct_version = buffer[17];
	
	func->busy_grp1 = (((uint16_t)buffer[18]) | ((uint16_t)buffer[19] << 8));
	func->busy_grp2 = (((uint16_t)buffer[20]) | ((uint16_t)buffer[21] << 8));
	func->busy_grp3 = (((uint16_t)buffer[22]) | ((uint16_t)buffer[23]  << 8));
	func->busy_grp4 = (((uint16_t)buffer[24])  | ((uint16_t)buffer[25]  << 8));
	func->busy_grp5 = (((uint16_t)buffer[26]) | ((uint16_t)buffer[27]  << 8));
	func->busy_grp6 = (((uint16_t)buffer[28]) | ((uint16_t)buffer[29]  << 8));
}
bool _host::switch_function(uint32_t device , uint32_t slot , bool speed,bool width,_function *func, _r1 *status)
{
	uint8_t buffer[0x40];	
	uint32_t argument 	= 0xFFFFFF;
	//uint16_t loop 		= 0x00;
	//uint32_t grp 		= 0x00;
	_function data_struct;
	
	/* Check the function supported or Busy*/
	argument = (((uint32_t)func->group1) | ((uint32_t)func->group2 << 4) | ((uint32_t)func->group3 << 8) | ((uint32_t)func->group4 << 12) | ((uint32_t)func->group5 << 16) | ((uint32_t)func->group6 << 20));
	
	if(!issue_switch_function( device , slot , speed, width, argument, status, &buffer[0] ))
		return false;

	extract_switch_function_data(&data_struct,&buffer[0]);
		
	if(data_struct.struct_version == 1)
	{
		argument = 0x00FFFFFF;
		if(data_struct.busy_grp1 == 0x00)
			argument |= ((uint32_t)func->group1);

		if(data_struct.busy_grp2 == 0x00)	
			argument |= ((uint32_t)func->group2 << 4);

		if(data_struct.busy_grp3 == 0x00)	
			argument |= ((uint32_t)func->group3 << 8);
		
		if(data_struct.busy_grp4 == 0x00)	
			argument |= ((uint32_t)func->group4 << 12);

		if(data_struct.busy_grp5 == 0x00)
			argument |= ((uint32_t)func->group5 << 16);
	
		if(data_struct.busy_grp6 == 0x00)	
			argument |= ((uint32_t)func->group6 << 20);
	}
	
	if(data_struct.current_grp1 == 0xF)
		argument |= (0xF);

	if(data_struct.current_grp2 == 0xF)	
		argument |= (0xF << 4);

	if(data_struct.current_grp3 == 0xF)	
		argument |= (0xF << 8);
	
	if(data_struct.current_grp4 == 0xF)	
		argument |= (0xF << 12);

	if(data_struct.current_grp5 == 0xF)
		argument |= (0xF << 16);
	
	if(data_struct.current_grp6 == 0xF)	
		argument |= (0xF << 20);
	
	func->busy_grp1 = data_struct.busy_grp1;	
	func->busy_grp2 = data_struct.busy_grp2;	
	func->busy_grp3 = data_struct.busy_grp3;
	func->busy_grp4 = data_struct.busy_grp4;	
	func->busy_grp5 = data_struct.busy_grp5;	
	func->busy_grp6 = data_struct.busy_grp6;
	
	argument |= (0x01 << 31);	

	if(!issue_switch_function( device , slot ,  speed, width, argument, status, &buffer[0] ))
		return false;

	extract_switch_function_data(&data_struct,&buffer[0]);
	
	func->current_grp1 = data_struct.current_grp1;	
	func->current_grp2 = data_struct.current_grp2;	
	func->current_grp3 = data_struct.current_grp3;
	func->current_grp4 = data_struct.current_grp4;	
	func->current_grp5 = data_struct.current_grp5;	
	func->current_grp6 = data_struct.current_grp6;
	
	return true;
}

bool _host::issue_switch_function(uint32_t device, uint32_t slot, bool speed,bool width,uint32_t argument,_r1 *status,uint8_t *ptr_buffer )
{
	uint32_t error = 0x00;	
	_r2 csd;
	if(!get_csd(&csd,&error))
		return false;
	if((csd.ccc() &  (0x01 << 10)) == 0x00 )
		return false;
	
	if(!select_card(status,&error))
		return false;

	if(!init_data_transfer(device , slot ,speed, width, 0x00 , 0x40))
		return false;
	
	if(!set_transfermode(false, true, 0x00, false, false, 0,0))
		return false;
    
	if(!did_host->p_card->switch_function(status,argument,&error))
		return false;
			
	if(!read_buffer(device , slot, 0x40, 0x00,ptr_buffer))
		return false;		

	env::SleepMs(150);
    if(!transfer_complete( &error))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error : "<<std::hex<<error;
		return false;
	}
	
	if(!deselect_card( status, &error))
		return false;
	return true;
}


bool _host::select_card( _r1 *status,uint32_t *error)
{
return (did_host->p_card->select_card(status, error));
}

bool _host::get_csd(_r2* ptr_csd,uint32_t *error)
{
return(did_host->p_card->get_csd(ptr_csd,error));
}

bool _host::get_ocr(_r3* ptr_ocr,uint32_t *error)
{
return(did_host->p_card->get_ocr(ptr_ocr,error));
}

bool _host::go_inactive(uint32_t *error)
{
return(did_host->p_card->go_inactive(error));
}

bool _host::asynchronous_abort(uint32_t device,uint32_t slot,_r1 *status,uint32_t *error)
{
	if(!p_card->stop_transmission(status,error))
		return false;
	if(!reset(SW_DAT_LINE_RESET, device, slot))
		return false;
	if(!reset(SW_CMD_LINE_RESET, device, slot))
		return false;		
	return true;
}

bool _host::synchronous_abort(uint32_t device,uint32_t slot,_r1 *status,uint32_t *error)
{
_reg sd_register;
	SET_SD_REGISTER_READ(&sd_register,HOST_BLOCK_GAP_CONTROL,device,slot);
	if(!read_host_reg(&sd_register))
	return false;
	sd_register.sd_reg.value |= 0x01; 
	if(!write_host_reg(&sd_register))
	return false;
	
	if(!transfer_complete( error))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error : "<<std::hex<<error<<endl;
		return false;
	}
	if(!p_card->stop_transmission(status,error))
		return false;
	if(!reset(SW_DAT_LINE_RESET, device, slot))
		return false;
	if(!reset(SW_CMD_LINE_RESET, device, slot))
		return false;		
	return true;
}

bool _host::set_normal_mode(uint32_t device, uint32_t slot, uint16_t speed_mode, uint16_t clock_selection_mode)
{
_function req_function,function;
_r1 status;
uint16_t mode = 0x00000000;
uint16_t mode_requested  = speed_mode;
uint16_t sd_spec_version = get_spec_version(device, slot);
uint32_t MaxFreq = 0x00;
while(mode_requested)
{
	mode++;
	mode_requested = mode_requested >> 1;
}
mode = mode - 1;
if(!get_supported_functions(device, slot, false,false, &req_function, &status))
	return false;
//	 std::cout << "Supported_grp1 :"<<  std::hex << req_function.group1<<std::endl; 
  HOST_LOG_DEBUG() << "Supported_grp1:" << std::hex << req_function.group1<<std::endl;
  HOST_LOG_DEBUG() << "Supported_grp2:" << std::hex << req_function.group2<<std::endl;
  HOST_LOG_DEBUG() << "Supported_grp3:" << std::hex << req_function.group3<<std::endl;
  HOST_LOG_DEBUG() << "Supported_grp4:" << std::hex << req_function.group4<<std::endl;
  HOST_LOG_DEBUG() << "Supported_grp5:" << std::hex << req_function.group5<<std::endl;
  HOST_LOG_DEBUG() << "Supported_grp6:" << std::hex << req_function.group6<<std::endl;
  HOST_LOG_DEBUG() << "current_grp1:" << std::hex << req_function.current_grp1<<std::endl;
  HOST_LOG_DEBUG() << "current_grp2:" << std::hex << req_function.current_grp2<<std::endl;
  HOST_LOG_DEBUG() << "current_grp3:" << std::hex << req_function.current_grp3<<std::endl;
  HOST_LOG_DEBUG() << "current_grp4:" << std::hex << req_function.current_grp4<<std::endl;
  HOST_LOG_DEBUG() << "current_grp5:" << std::hex << req_function.current_grp5<<std::endl;
  HOST_LOG_DEBUG() << "current_grp6:" << std::hex << req_function.current_grp6<<std::endl;
	

if((req_function.group1 & speed_mode) != speed_mode)
	{
		HOST_LOG_DEBUG()<<"Mode not supported by the card"<<endl;
		return false;
	}

	 function.group1 = mode;
	 function.group2 = 0x00;
	 function.group3 = 0x00;
	 function.group4 = 0x00;
	 function.group5 = 0x00;
	 function.group6 = 0x00;
	 HOST_LOG_DEBUG()<< "We are setting mode  to :"<<  std::hex << function.group1<<std::endl;
	if(!switch_function(device , slot, false, false, &function, &status))
	{
		HOST_LOG_DEBUG()<<"Mode switching failed"<<endl;
		return false;
	}
  HOST_LOG_DEBUG() << "current_grp1:" << std::hex << function.current_grp1<<std::endl;
  HOST_LOG_DEBUG() << "current_grp2:" << std::hex << function.current_grp2<<std::endl;
  HOST_LOG_DEBUG() << "current_grp3:" << std::hex << function.current_grp3<<std::endl;
  HOST_LOG_DEBUG() << "current_grp4:" << std::hex << function.current_grp4<<std::endl;
  HOST_LOG_DEBUG() << "current_grp5:" << std::hex << function.current_grp5<<std::endl;
  HOST_LOG_DEBUG() << "current_grp6:" << std::hex << function.current_grp6<<std::endl;
	 

switch(speed_mode)
{
	case DEFAULT_SPEED:
		MaxFreq = DS_MAX_FREQ;
		set_highspeed(false, device, slot);
	break;
	case HIGH_SPEED:
		MaxFreq = HS_MAX_FREQ;
		set_highspeed(true, device, slot);
	break;
}	
 

if(PRESET_CLK_SELECTION_MODE == clock_selection_mode)
{
	if(HOST_SPEC_VERSION_3_00 > sd_spec_version)
	{
		HOST_LOG_DEBUG()<<"Spec version"<<std::dec<<sd_spec_version+1<<"will not support Clock Preset"<<endl;
		return false;
	}

	_reg sd_register;
	SET_SD_REGISTER_READ(&sd_register,HOST_HOST_CONTROL_2,device,slot);
	read_host_reg(&sd_register);
	sd_register.sd_reg.value = (sd_register.sd_reg.value | 0x8000);
	write_host_reg(&sd_register);
}
else if((DIVIDED_CLK_SELECTION_MODE == clock_selection_mode) || (PROGRAMABLE_CLK_SELECTION_MODE == clock_selection_mode))
{

	if(!set_clock(device, slot,MaxFreq,(uint32_t)clock_selection_mode))
	return false;
}
	 
return true;	
}
 bool _host::set_UHS_mode(uint32_t device, uint32_t slot, uint16_t speed_mode, uint16_t clock_selection_mode)
 {
 
 _function req_function,function;
_r1 status;
uint16_t mode = 0x00000000;
uint16_t mode_requested  = speed_mode;
uint16_t sd_spec_version = get_spec_version(device, slot);
uint32_t MaxFreq = 0x00;
_reg sd_register;
reg32 cap_register1;

SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES_1,device,slot);
read_host_reg(&sd_register);
cap_register1.reg_val = sd_register.sd_reg.value;


SET_SD_REGISTER_READ(&sd_register,HOST_HOST_CONTROL_2,device,slot);
read_host_reg(&sd_register);


if((sd_register.sd_reg.value & 0x0008) != 0x0008)
{
		HOST_LOG_DEBUG()<<"Host is not in UHS mode "<<endl;
		return false;	
}



while(mode_requested)
{
	mode++;
	mode_requested = mode_requested >> 1;
}
mode--;
if(!get_supported_functions(device, slot, false,true, &req_function, &status))
	return false;
	 HOST_LOG_DEBUG()<< "Supported_grp1 :"<<  std::hex << req_function.group1<<std::endl; 
     	HOST_LOG_DEBUG()<< "Supported_grp2 :"<<  std::hex << req_function.group2<<std::endl; 	 
     HOST_LOG_DEBUG() << "Supported_grp3 :"<<  std::hex << req_function.group3<<std::endl; 
     HOST_LOG_DEBUG()<< "Supported_grp4 :"<<  std::hex << req_function.group4<<std::endl; 	 
     HOST_LOG_DEBUG() << "Supported_grp5 :"<<  std::hex << req_function.group5<<std::endl; 
     HOST_LOG_DEBUG()<< "Supported_grp6 :"<<  std::hex << req_function.group6<<std::endl; 
	 HOST_LOG_DEBUG()<< "current_grp1 :"<<  std::hex << req_function.current_grp1<<std::endl; 
     HOST_LOG_DEBUG()<< "current_grp2 :"<<  std::hex << req_function.current_grp2<<std::endl; 	 
     HOST_LOG_DEBUG()<< "current_grp3 :"<<  std::hex << req_function.current_grp3<<std::endl; 
     HOST_LOG_DEBUG()<< "current_grp4 :"<<  std::hex << req_function.current_grp4<<std::endl; 	 
     HOST_LOG_DEBUG()<< "current_grp5 :"<<  std::hex << req_function.current_grp5<<std::endl; 
     HOST_LOG_DEBUG()<< "current_grp6 :"<<  std::hex << req_function.current_grp6<<std::endl; 	 
	

if((req_function.group1 & speed_mode) != speed_mode)
	{
		HOST_LOG_DEBUG()<<"Mode not supported by the card"<<endl;
		return false;
	}

	 function.group1 = mode;
	 function.group2 = 0x00;
	 function.group3 = 0x00;
	 function.group4 = 0x00;
	 function.group5 = 0x00;
	 function.group6 = 0x00;
	 HOST_LOG_DEBUG()<< "We are setting mode  to :"<<  std::hex << function.group1<<std::endl;
	 
	 //switch_function(uint32_t device , uint32_t slot , bool speed,bool width,_function *func, _r1 *status)
if(!switch_function(device , slot, false, true, &function, &status))
	{
		 HOST_LOG_DEBUG()<<"Mode switching failed"<<endl;
		return false;
	}
	 HOST_LOG_DEBUG()<< "current_grp1 :"<<  std::hex << function.current_grp1<<std::endl; 
     HOST_LOG_DEBUG()<< "current_grp2 :"<<  std::hex << function.current_grp2<<std::endl; 	 
     HOST_LOG_DEBUG()<< "current_grp3 :"<<  std::hex << function.current_grp3<<std::endl; 
     HOST_LOG_DEBUG()<< "current_grp4 :"<<  std::hex << function.current_grp4<<std::endl; 	 
     HOST_LOG_DEBUG()<< "current_grp5 :"<<  std::hex << function.current_grp5<<std::endl; 
     HOST_LOG_DEBUG()<< "current_grp6 :"<<  std::hex << function.current_grp6<<std::endl; 
 

switch(speed_mode)
{
	case SDR_12:
		MaxFreq = SDR12_MAX_FREQ;
		set_uhspeedmode(device, slot,(uint8_t)mode);
	break;
	case SDR_25:
		MaxFreq = SDR25_MAX_FREQ;
		set_uhspeedmode(device, slot,(uint8_t)mode);
	break;
	case SDR_50:
		MaxFreq = SDR50_MAX_FREQ;
		set_uhspeedmode(device, slot ,(uint8_t)mode);
		if(cap_register1.bit.b13 == 1)
			setup_tuning(device,slot);
	break;
	case SDR_104:
		MaxFreq = SDR104_MAX_FREQ;
		set_uhspeedmode(device, slot , (uint8_t)mode);
		setup_tuning(device,slot);
	break;
	case DDR_50:
		MaxFreq = DDR50_MAX_FREQ;
		set_uhspeedmode(device, slot , (uint8_t)mode);
	break;
	
}	


if(PRESET_CLK_SELECTION_MODE == clock_selection_mode)
{
	if(HOST_SPEC_VERSION_3_00 > sd_spec_version)
	{
		HOST_LOG_DEBUG()<<"Spec version"<<std::dec<<sd_spec_version+1<<"will not support Clock Preset"<<endl;
		return false;
	}

	_reg sd_register;
	SET_SD_REGISTER_READ(&sd_register,HOST_HOST_CONTROL_2,device,slot);
	read_host_reg(&sd_register);
	sd_register.sd_reg.value = (sd_register.sd_reg.value | 0x8000);
	write_host_reg(&sd_register);
}
else if((DIVIDED_CLK_SELECTION_MODE == clock_selection_mode) || (PROGRAMABLE_CLK_SELECTION_MODE == clock_selection_mode))
{

	if(!set_clock(device, slot,MaxFreq,clock_selection_mode))
	return false;
}
env::SleepMs(50);

SET_SD_REGISTER_READ(&sd_register,HOST_CLOCK_CONTROL,device,slot);
read_host_reg(&sd_register);
HOST_LOG_DEBUG()<<"Preset Clock Ctrl Value : "<< sd_register.sd_reg.value<<endl;

if(speed_mode == SDR_104)
{
	if(!tune(device,slot))
	{
		HOST_LOG_DEBUG()<<"Tuning Failed in SDR104 Mode";
		return false;
	}
}
else if(speed_mode == SDR_50)
{
	if(cap_register1.bit.b13 == 1)
	{
		if(!tune(device,slot))
		{
			HOST_LOG_DEBUG()<<"Tuning Failed in SDR50 Mode"<<endl;
			return false;
		}
	}	
}
return true;
 
 }
 
bool _host::check_tuning_valid(uint32_t device, uint32_t slot)
{
_function function;
reg32 cap_register1;
_reg sd_register;
_r1 status;
SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES_1,device,slot);
read_host_reg(&sd_register);
cap_register1.reg_val = sd_register.sd_reg.value;

if(!get_supported_functions(device, slot, false,true, &function, &status))
	return false;
	
if(cap_register1.bit.b0 == 0 || cap_register1.bit.b1 == 0)
{
	HOST_LOG_DEBUG()<<"SDR104 / SDR50 Modes are not supported by the host Tuning not supported"<<endl;
	return false;
}	
	
if( (function.current_grp1 == 0x02) || (function.current_grp1 == 0x03) )
{
	if((function.current_grp1 == 0x02) && (cap_register1.bit.b13 == 0))
	{
		HOST_LOG_DEBUG()<<"Tuning not required for SDR50 Modes in this Host configuration"<<endl;
		return false;	
	}
}
else
{
	HOST_LOG_DEBUG()<<"Card not in SDR104 / SDR50 Mode Tuning not required for the current speed mode"<<endl;
	return false;
}

SET_SD_REGISTER_READ(&sd_register,HOST_HOST_CONTROL_2,device,slot);
read_host_reg(&sd_register);

if((sd_register.sd_reg.value & 0x000A) != 0x000A)
{
	if( (sd_register.sd_reg.value & 0x000B) != 0x000B) 
	{
		HOST_LOG_DEBUG()<<"Host is not in SDR104/SDR50 mode Tuning not supported in this mode"<<endl;
		return false;	
	}
}
return true;	
} 


bool _host::issue_tuning(uint32_t device, uint32_t slot)
{
if(!check_tuning_valid(device,slot))
	return false;

if(!tuning_procedure(device,slot))
	return false;
	
return true;	
}

bool _host::setup_tuning(uint32_t device,uint32_t slot)
{
_r1 status;
uint32_t error;

	if(!select_card(&status,&error))
	return false;
	
	if(get_card_status( &status, &error))
	{
		if(tran == status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in Trans state"<<endl;	
	}
	
	if(!init_data_transfer(device , slot ,true, true, 0x00 , 0x40))
		return false;
	
	if(!set_transfermode(false, true, 0x00, false, false, device,slot))
		return false;
	
	return true;
}
bool _host::tune(uint32_t device,uint32_t slot)
{
uint32_t interrupt=0x00;
_r1 status;
uint32_t error;
uint32_t loop2exit = 10, loop1exit = 50;
_reg sd_register;

	
/* Set Execute tuning*/
SET_SD_REGISTER_READ(&sd_register,HOST_HOST_CONTROL_2,device,slot);
read_host_reg(&sd_register);
sd_register.sd_reg.value = sd_register.sd_reg.value | 0x0040;
write_host_reg(&sd_register);
do{
	if(!p_card->send_tuning_block(&status,&error))
		return false;
	/* Read Int status check Buffer read ready interrupt*/
	do{
	interrupt = get_int_status(device , slot);
 	HOST_LOG_DEBUG()<<"Interrupt Status :"<<std::hex<<interrupt <<endl;
	}while((!(interrupt & 0x0020)) && (loop2exit--));
	if(!loop2exit)
		return false;
	loop2exit = 10;
	read_host_reg(&sd_register);
}while((sd_register.sd_reg.value & 0x0040) && (loop1exit--));

	if(!loop1exit)
		return false;

read_host_reg(&sd_register);
if(sd_register.sd_reg.value & 0x0080)
{
	if(!deselect_card( &status, &error))
		return false;
	return true;
}
return false;

}

bool _host::tuning_procedure(uint32_t device,uint32_t slot)
{
uint32_t interrupt=0x00;
_r1 status;
uint32_t error;
uint32_t loop2exit = 10, loop1exit = 50;
_reg sd_register;

	if(!select_card(&status,&error))
	return false;
	
	if(get_card_status( &status, &error))
	{
		if(tran == status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in Trans state"<<endl	;	
	}
	
	if(!init_data_transfer(device , slot ,false, true, 0x00 , 0x40))
		return false;
	
	if(!set_transfermode(false, true, 0x00, false, false, device,slot))
		return false;
	
/* Set Execute tuning*/
SET_SD_REGISTER_READ(&sd_register,HOST_HOST_CONTROL_2,device,slot);
read_host_reg(&sd_register);
sd_register.sd_reg.value = sd_register.sd_reg.value | 0x0040;
write_host_reg(&sd_register);
do{
	if(!p_card->send_tuning_block(&status,&error))
		return false;
	/* Read Int status check Buffer read ready interrupt*/
	do{
	interrupt = get_int_status(device , slot);
	HOST_LOG_DEBUG()<<"Interrupt Status :"<<std::hex<<interrupt<<endl;
	}while((!(interrupt & 0x0020)) && (loop2exit--));
	if(!loop2exit)
		return false;
	loop2exit = 10;
	read_host_reg(&sd_register);
}while((sd_register.sd_reg.value & 0x0040) && (loop1exit--));

	if(!loop1exit)
		return false;

read_host_reg(&sd_register);
if(sd_register.sd_reg.value & 0x0080)
{
	if(!deselect_card( &status, &error))
		return false;
	return true;
}
return false;
}




uint8_t _host::get_spec_version(uint32_t device, uint32_t slot)
{
_reg sd_register;
SET_SD_REGISTER_READ(&sd_register,HOST_HOST_VERSION,device,slot);
read_host_reg(&sd_register);
return ((uint8_t)(sd_register.sd_reg.value & 0x00FF));
}
uint8_t _host::get_vendor_version(uint32_t device, uint32_t slot)
{
_reg sd_register;
SET_SD_REGISTER_READ(&sd_register,HOST_HOST_VERSION,device,slot);
read_host_reg(&sd_register);
return ((uint8_t)((sd_register.sd_reg.value & 0xFF00) >> 8));
}

uint32_t _host::calculate_freq_select(uint32_t device, uint32_t slot,bool clk_generator, uint16_t speed_mode)
{
uint32_t base_clk = 0x00;
uint32_t clk_multiplier = 0x00;
uint32_t spec_version = get_spec_version( device, slot);
_reg sd_register;

SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES,device,slot);
read_host_reg(&sd_register);
base_clk = ((sd_register.sd_reg.value & 0x0000FF00) >> 8);
if(HOST_SPEC_VERSION_3_00 == spec_version)
{
	SET_SD_REGISTER_READ(&sd_register,HOST_CAPABILITIES_1,device,slot);
	read_host_reg(&sd_register);
	clk_multiplier = ((sd_register.sd_reg.value & 0x00FF0000) >> 16);
}
else
base_clk = base_clk & 0x3F;

if(!clk_generator)
clk_multiplier = 1;

	switch(speed_mode)
	{
		case DEFAULT_SPEED:
			return ((base_clk*clk_multiplier)/DS_MAX_FREQ);
		break;
		
		case HIGH_SPEED:
			return ((base_clk*clk_multiplier)/HS_MAX_FREQ);
		break;
		
		case SDR_50:
			return ((base_clk*clk_multiplier)/SDR50_MAX_FREQ);
		break;
		
		case SDR_104:
			return ((base_clk*clk_multiplier)/SDR104_MAX_FREQ);
		break;
		
		case DDR_50:
			return ((base_clk*clk_multiplier)/DDR50_MAX_FREQ);
		break;
		default:
		return ((base_clk*clk_multiplier)/DS_MAX_FREQ);
	}
}

bool _host::error_interrupt_recovery(uint32_t device, uint32_t slot,uint32_t error)
{
int_signal_enable(device , slot, INT_SIGNAL_ENABLE);
uint32_t abort_cmd_error;

if(error & 0x000F)
{
	HOST_LOG_DEBUG()<<"CMD line error occured"<<endl;
	if(!reset(SW_CMD_LINE_RESET, device, slot))
	return false;
}

if(error & 0x0070)
{
	HOST_LOG_DEBUG()<<"DAT line error occured"<<endl;
	if(!reset(SW_DAT_LINE_RESET, device, slot))
	return false;
}

_r1 status;
if(!p_card->stop_transmission(&status,&abort_cmd_error))
{
	if(get_card_status( &status, &error))
	{
		if(tran != status.bit.current_state)
			return false;
	}
}
env::SleepUs(50);

_reg prsnt_state_register;
reg32 present_state;
SET_SD_REGISTER_READ(&prsnt_state_register,HOST_PRESENT_STATE,device,slot);
if(!read_host_reg(&prsnt_state_register))
  return false;
present_state.reg_val = prsnt_state_register.sd_reg.value;

if((present_state.bit.b20) && (present_state.bit.b21) && (present_state.bit.b22) && (present_state.bit.b23))
	return true;

return false;
}

bool _host::direct_io_read(uint32_t function,uint32_t reg_offset, _r5 *status,uint32_t *error)
{
return (p_card->direct_io_read(function,reg_offset, status,error));
}
bool _host::direct_io_write(bool raw , uint32_t function,uint32_t reg_offset, uint32_t data ,_r5 *status,uint32_t *error)
{
return (p_card->direct_io_write(raw,function,reg_offset,data, status,error));
}
bool _host::extended_io_read(uint32_t device, uint32_t slot , uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address,bool block_mode, uint32_t count,uint8_t *ptr_buffer, _r5 *status,uint32_t *error)
{
	//uint8_t buswidth = 0x00;

	if(!set_timeout_ctrl(0x0E,0,0))
	return false;

	if(!set_host_buswidth( 0x00, 0,0))
	return false;
	
	if(!set_speed(true, 0, 0))
		return false;

	if(!set_host_block_length_n_count( count,  0x00, device,slot))
	return false;
	
	if(!set_transfermode(false, true, 0x00, false, false, 0,0))
		return false;

	if(!p_card->extended_io_read(function, reg_offset, fixe_or_inc_address,block_mode, count, status,error))
	{
		HOST_LOG_DEBUG()<<"Extended IO Read failed "<<endl;
		return false;
	}
	env::SleepMs(100);
	if(!read_buffer(device, slot , count ,0, ptr_buffer))
		return false;
	
	env::SleepMs(100);
	
	if(!transfer_complete( error , 0x10))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error : "<<std::hex<<*error<<endl;
		return false;
	}	
	return true;
}
bool _host::extended_io_write(uint32_t device, uint32_t slot ,uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address,bool block_mode, uint32_t count,uint8_t *ptr_buffer, _r5 *status,uint32_t *error)
{
	//uint8_t buswidth = 0x00;

	if(!set_timeout_ctrl(0x0E,device,slot))
	return false;

	if(!set_host_buswidth( 0x00, device,slot))
	return false;
	
	if(!set_speed(true, device,slot))
		return false;

	if(!set_host_block_length_n_count( count,  0x00, device,slot))
	return false;
	
	if(!set_transfermode(false, false, 0x00, false, false, 0,0))
		return false;

	if(!p_card->extended_io_write(function, reg_offset, fixe_or_inc_address,block_mode, count, status,error))
	{
		HOST_LOG_DEBUG()<<"Extended IO Read failed ";
		return false;
	}
	env::SleepMs(100);
	if(!write_buffer(device, slot , count ,0, ptr_buffer))
		return false;
	
	env::SleepMs(100);
	
	if(!transfer_complete( error , 0x10))
	{
		HOST_LOG_DEBUG()<<"TRNS complete error : "<<std::hex<<*error;
		return false;
	}	
	return true;
}
bool _host::recover_normal_error(uint32_t device , uint32_t slot)
{
  uint32_t int_ststus = 0x00;
  _r1 status;
  uint32_t error;
  _reg reg;
  reg32 present_state_reg;
  
  
  if(!int_signal_enable(device , slot, INT_SIGNAL_ENABLE))
    return false; 

  int_ststus = get_int_status(device , slot);	
	
  if((int_ststus >> 16) & 0x0F)
  {
	if(!reset(SW_CMD_LINE_RESET,device,slot))
		return false; 
  }
	
  if((int_ststus >> 16) & 0x70)	
  {
	if(!reset(SW_DAT_LINE_RESET,device,slot))
		return false; 
  }	
  
  if(!asynchronous_abort(device,slot,&status,&error))
  	return false; 
   
   env::SleepMs(2); 
   
     /*Read Present State Register*/
   SET_SD_REGISTER_READ(&reg,HOST_PRESENT_STATE,device,slot);
   if(!read_host_reg(&reg))
     return false;
   present_state_reg.reg_val = reg.sd_reg.value;
   if((present_state_reg.bit.b20 != 1) || (present_state_reg.bit.b21 != 1) || (present_state_reg.bit.b22 != 1) || (present_state_reg.bit.b23 != 1))
   {
	 HOST_LOG_DEBUG()<<"DAT lines are not driven High"<<endl;
     return false; 
   }
   
   return true;
}

bool _host::datatransfer_error_inject(uint32_t device, uint32_t slot,bool speed , uint32_t error_type, bool data_transfer_direction,uint32_t *ptr_error)
{
	_r1 status;
	uint32_t error = 0x00;
	//uint32_t block_trnsfd = 0x00;
	//uint32_t irq_status = 0x00;
	bool blk_count_enable = true;
	bool dma_enable = false;
	//bool data_transfer_direction = false;
	uint8_t data_buffer[1025];
	uint32_t blk_size = 0x200;
	uint32_t blk_count = 0x02;
	bool bus_width = true;
	uint32_t card_address = 0x4000;
	uint32_t autocmd = 0x01;
	

	if(!select_card(&status,&error))
	return false;
	
	if(get_card_status( &status, &error))
	{
		if(tran == status.bit.current_state)
			HOST_LOG_DEBUG()<<"Card is in Trans state"<<endl;	
	}

	if(error_type == LESS_BLOCK_SIZE)
		blk_size     = 0x1;
	
	if(error_type == CARD_ADDRESS_ERROR)
	{	
		card_address = 0xFFFFFFFF;
		HOST_LOG_DEBUG()<<"Card Address : "<<std::hex<<card_address;
	}
	
	if(error_type == AUTO_CMD23_WITHOUT_BLOCK_COUNT)
	{	
		autocmd = 0x02;
		//dma_select(0x02,device, slot); // old call was: dma_select(0x01,device, slot); refer: host control1 reg spec, bit3 & bit4
		//dma_enable = true;
		HOST_LOG_DEBUG()<<"Auto Command : "<<std::hex<<autocmd<<endl;
	}


	
	if(!init_data_transfer(device , slot ,speed, bus_width, blk_count , blk_size ))
		return false;	
	
	if(error_type == DATA_WIDTH_MISMATCH)
	{
		if(!set_host_buswidth( (!bus_width), device,slot))
			return false;
	}
	
	//dma_select(0x01,device, slot);
	//if(!set_host_buswidth( (!bus_width), device,slot))
	//return false;
	
	if(!set_transfermode(blk_count_enable , data_transfer_direction, autocmd , blk_count_enable , dma_enable, device,slot))
	return false;
	
	
		
	if(autocmd == 0x02)
	{
		_reg reg;
		SET_SD_REGISTER_WRITE(&reg,HOST_DMA_ADDRESS,device,slot,0x00);
		write_host_reg(&reg);
	}

	if(data_transfer_direction == false) //Write
	{
		if(blk_count == 0x00)
		{
			if(!write_singleblock(card_address,&status,&error))
			{
				*ptr_error = error;	
				return false;		
			}	
		}
		else
		{
			if(!write_multiblock(card_address,&status,&error))
			{
				*ptr_error = error;	
				return false;		
			}
		}
	}
	else
	{
		if(blk_count == 0x00)
		{
			if(!read_singleblock( card_address,&status,&error))
			{
				*ptr_error = error;	
				return false;		
			}
		}
		else
		{
			if(!read_multiblock(card_address,&status,&error))
			{
				*ptr_error = error;	
				return false;		
			}
		}	
	}
	
	if(error_type == WRONG_CMD_INDEX)
	{
		if(!write_multiblock_error(card_address,&status,&error))
		{
			*ptr_error = error;	
			return false;		
		}
	}		
	if(error_type == CLOCK_ERROR)
	{	
		card_clk(false, device, slot);
	}
	env::SleepMs(100);
	if(data_transfer_direction == false) //Write
	{
		if(!write_buffer(device, slot, blk_size, blk_count, &data_buffer[0]))
		{
			*ptr_error = error;	
			return false;		
		}
	}
	else
	{
		if(!read_buffer(device, slot, blk_size, blk_count, &data_buffer[0]))
		{
			*ptr_error = error;	
			return false;		
		}
	}
	if(error_type == CLOCK_ERROR)
	{	
		card_clk(true, device, slot);
		env::SleepMs(100);		
		if(data_transfer_direction == false) //Write
		{
			if(!write_buffer(device, slot, blk_size, blk_count, &data_buffer[0]))
				return false;
		}
		else
		{
			if(!read_buffer(device, slot, blk_size, blk_count, &data_buffer[0]))
				return false;		
		}
	}

	*ptr_error = error;	
	env::SleepMs(100);
	return true;
}
/****************************************************************************/
 uint8_t mmc_crc7 (uint8_t *data, uint8_t length)
 {
    uint8_t i, ibit, c, crc;
    crc = 0x00;                                                                // Set initial value
 
    printf("CRC Data\n");
    for (i = 0; i < length; i++, data++)
    {
       c = *data;
       printf("0x%x ,",c);
 
       for (ibit = 0; ibit < 8; ibit++)
       {
          crc = crc << 1;
          if ((c ^ crc) & 0x80) 
			crc = crc ^ 0x09;                              // ^ is XOR
          c = c << 1;
       }
 
        crc = crc & 0x7F;
    }
 
    //shift_left(&crc, 1, 1);                                                    // MMC card stores the result in the top 7 bits so shift them left 1
                                                                               // Should shift in a 1 not a 0 as one of the cards I have won't work otherwise
    printf("\n");
	return crc;
 }

/****************************************************************************/
bool _host::write_csd(uint32_t device, uint32_t slot,uint8_t *csd_data,bool speed, bool bus_width ,_r1 *status)
{
	//_reg sd_register;
	//reg32 present_state;
	//uint32_t data = 0x00
	uint32_t error = 0x00;
	uint8_t total_data_size = 16;
	//uint32_t reg = HOST_BUFFER_0;

	if(!select_card(status,&error))
		return false;

	if(!init_data_transfer(device , slot ,speed, bus_width, 0x00 , total_data_size))
		return false;
	
	if(!set_transfermode(false, false, 0x00, false, false, 0,0))
	
		return false;
	
	if(!program_csd(status,&error))
		return false;
	//csd_data[0] = ( csd_data[0] & 0xEF);
	
	uint8_t crc = mmc_crc7(csd_data , 15);
	printf("CRC = %X\n", crc);
	
	uint8_t wide_data[18];
	uint32_t loop = 0;
	
	for (loop = 0 ; loop < 15 ; loop++)
	{
			wide_data[14 - loop] = csd_data[loop];
	}	
	
    wide_data[15] = ((crc << 1) | (0x01));

	printf("Complete Data\n");
	for (loop = 0 ; loop < 15 ; loop++)
	{
			printf("%X ", wide_data[15 - loop]);
	}	
	printf("\n");
	if(get_card_status( status,&error))
	{
		if(rcv == status->bit.current_state)
		printf("Card is in Receive data state\n")	;	
		else
		{
			printf("Card not in Receive data state\n");
			return false;			
		}
	}
	write_buffer( device, slot, total_data_size , 0x00 , &wide_data[0]);
	
    env::SleepMs(100);
    if(!transfer_complete( &error))
	{
		printf("TRNS complete error : %x\n",error);
		return false;
	}
	if(!deselect_card( status,&error))
		return false;
	return true;
}

bool _host::write_csd_1_1_29(uint32_t device, uint32_t slot,uint8_t *csd_data,bool speed, bool bus_width ,_r1 *status)
{
	//_reg sd_register;
	//reg32 present_state;
	//uint32_t data = 0x00;
	uint32_t error = 0x00;
	uint8_t total_data_size = 16;
	//uint32_t reg = HOST_BUFFER_0;

	if(!select_card(status,&error))
		return false;

	if(!init_data_transfer(device , slot ,speed, bus_width, 0x00 , total_data_size))
		return false;
	
	if(!set_transfermode(false, false, 0x00, false, false, 0,0))
	
		return false;
	
	if(!program_csd(status,&error))
		return false;
	//csd_data[0] = ( csd_data[0] & 0xEF);
	
	uint8_t wide_data[18];
	uint32_t loop = 0;
	
	HOST_LOG_DEBUG() <<"Complete Data :" << std::endl;
	for (loop = 0 ; loop < 15 ; loop++)
	{
			wide_data[14-loop] = csd_data[loop];
	}	
	for (loop = 0 ; loop < 15 ; loop++)
	{
			HOST_LOG_DEBUG() <<"  "<< std::hex << wide_data[15-loop] <<std::endl;
	}	
	HOST_LOG_DEBUG() <<"\n" << std::endl;
	
	uint8_t crc = mmc_crc7(&wide_data[0] , 15);
	HOST_LOG_DEBUG() <<"write_csd_1_1_29: CRC = " <<std::hex <<  crc <<std::endl;
	
	//crc = ~crc;
    wide_data[15] = ((crc << 1) | (0x01));
    //wide_data[15] = ((0x0 << 1) | (0x01));
	HOST_LOG_DEBUG() <<"write_csd_1_1_29: Changed CRC =  " <<std::hex << wide_data[15] <<std::endl;
	if(get_card_status( status,&error))
	{
		if((rcv == status->bit.current_state) && (!status->bit.com_crc_error))
		HOST_LOG_DEBUG() <<"Card is in Receive data state: status->bit.com_crc_error: "<<std::hex<<status->bit.com_crc_error<<std::endl	;	
		else
		{
			HOST_LOG_DEBUG() <<"Card not in Receive data state: com_crc_error:: " <<std::hex<<status->bit.com_crc_error <<std::endl;
			return false;			
		}
	}
	write_buffer( device, slot, total_data_size , 0x00 , &wide_data[0]);
	
    env::SleepMs(100);
    if(!transfer_complete( &error))
	{
		HOST_LOG_DEBUG() <<"TRNS complete error : "<<std::hex << error<<std::endl;
		return false;
	}
	if(get_card_status( status,&error))
	{
		if(!status->bit.com_crc_error)
		HOST_LOG_DEBUG() <<"No Command CRC Error "<<std::endl;	
		else
		{
			HOST_LOG_DEBUG() <<"Command CRC Error occured during previous command execution "<< std::endl;
			return false;			
		}
	}
	if(!deselect_card( status,&error))
		return false;
	return true;
}

bool _host::send_num_wr_blocks(_r1 *status, uint32_t *error)
{
return(did_host->p_card->send_num_wr_blocks(status,error)); 
}


/****************************************************************************  API TO EXPORT *******************************************************************************************/
/****************************************************************************  API TO EXPORT *******************************************************************************************/
/****************************************************************************  API TO EXPORT *******************************************************************************************/



/**************************************************************************
 *	read_host_reg(_reg *preg)
 *	Access to MMIO space
 *	Argument is _reg contains the register offset, width, device number
 *  Slot number valu
**************************************************************************/
bool read_host_reg(_reg *preg)
{
  return rhost_reg(preg);
}
/**************************************************************************
 *	write_host_reg(_reg *preg)
 *	Access to MMIO space
 *	Argument is _reg contains the register offset, width, device number
 *  Slot number valu
**************************************************************************/
bool write_host_reg(_reg *preg)
{
  return whost_reg(preg);
}



	/**************************************************************************
	 *	init_driver()
	 *	It create basic object "did_host" of the Host
	 *	This Object is used to controll all Host regiters
	 *  Return true if success
	**************************************************************************/
	bool init_driver(uint32_t pci_bus, uint32_t pci_device, uint32_t pci_function)
	{
		#if SD_PCI
			did_host = new _host( pci_bus,  pci_device, pci_function);
		#endif
		
		#if SD_MMIO
			did_host = new _host( );
		#endif
		
		HOST_LOG_DEBUG() <<"HOST:init_driver"<<"bus=" << std::dec << pci_bus <<"  pci-device="<<std::hex<<pci_device<<"  function="<<std::hex<<pci_function<<std::endl;
		return((did_host == NULL) ? false : true);
	}
	
#if SD_PCI
	/**************************************************************************
	 *	read_pcicfg(_reg *preg)
	 *	Access to PCICFG space
	 *	Argument is _reg contains the register offset, width, device number
	 *  Slot number valu
	**************************************************************************/
	uint64_t read_pcicfg(uint32_t device , uint32_t offset , uint32_t width)
	{
	  //char *name		 	= sd_register[host_offset].name;
	_reg sd_pci_register_def;
	sd_pci_register_def.pci_or_mmio_reg.value		= 0x00;
	sd_pci_register_def.pci_or_mmio_reg.offset		= offset;
	sd_pci_register_def.pci_or_mmio_reg.width		= width;
	sd_pci_register_def.pci_or_mmio_reg.device		= device;

	  if(did_host == NULL)
	  {
		HOST_LOG_DEBUG() << "Host Driver Not initialized" << std::endl;
		exit(1);
	  }
	  
	 did_host->p_pci_handler->read_pci(&sd_pci_register_def);
	 return sd_pci_register_def.pci_or_mmio_reg.value;
	}
	/**************************************************************************
	 *	write_pcicfg(_reg *preg)
	 *	Access to PCICFG space
	 *	Argument is _reg contains the register offset, width, device number
	 *  Slot number valu
	**************************************************************************/
	void write_pcicfg(uint32_t device , uint32_t offset , uint32_t width , uint64_t value)
	{
	 // char* reg_name = sd_register[host_offset].name;
	_reg sd_pci_register_def;
	sd_pci_register_def.pci_or_mmio_reg.value		= value;
	sd_pci_register_def.pci_or_mmio_reg.offset		= offset;
	sd_pci_register_def.pci_or_mmio_reg.width		= width;
	sd_pci_register_def.pci_or_mmio_reg.device		= device;

	  if(did_host == NULL)
	  {
		HOST_LOG_DEBUG()<<"Host Driver Not initialized"<<endl;
		exit(1);
	  }
	  did_host->p_pci_handler->write_pci(&sd_pci_register_def);

	}

	/**************************************************************************
	 *	get_total_pcidevice(void)
	 *	Get total SD Pci device in the syatem
	**************************************************************************/
	uint8_t get_total_pcidevice(void)
	{
	 if (did_host!=NULL)
	{
	   if ( did_host->p_pci_handler != NULL)
	{
	  return (did_host->p_pci_handler->get_total_dev());
		}
	 }
	 return 0;  
	}
	/**************************************************************************
	 *	get_total_pcidevice(void)
	 *	Get total SD Pci device in the syatem
	**************************************************************************/
	uint8_t get_total_slot(uint32_t device)
	{
	  return (did_host->p_pci_handler->get_total_slot(device));
	}
	/**************************************************************************
	 *	get_pciheader(_pci_type0_header *header_ptr, uint32_t dev_number)
	 *	Get PCI header of the SD device
	**************************************************************************/
	bool get_pciheader(_pci_type0_header *header_ptr, uint32_t dev_number)
	{
	  return (did_host->p_pci_handler->get_pci_header(header_ptr, dev_number));
	}

#endif



/**************************************************************************
 *	sw_host_reset(uint8_t reset_type, uint32_t device, uint32_t slot)
 *	Do S/W reset for the Host mentioned bu device number and slot
 *  Argument used to specify diffrent type of reset 
 *  SW_DAT_LINE_RESET , SW_CMD_LINE_RESET , SW_ALL_LINE_RESET 
**************************************************************************/
bool sw_host_reset(uint8_t reset_type, uint32_t device, uint32_t slot)
{
  return (did_host->reset(reset_type, device, slot));
}
/**************************************************************************
 *	detect_card(uint32_t device, uint32_t slot)
 *	This is to detect card in the device and slot mentioned in the argument
**************************************************************************/
bool detect_card(uint32_t device, uint32_t slot)
{
  return(did_host->detect(device, slot));
}
/**************************************************************************
 *	card_writeprotected(uint32_t device, uint32_t slot)
 *	This is to detect weather the card is write prtected
**************************************************************************/
bool card_writeprotected(uint32_t device, uint32_t slot)
{
  return(did_host->writeprotected(device, slot));
}
/**************************************************************************
 *	set_card_voltage(uint8_t voltage, uint32_t device, uint32_t slot)
 *	set the given voltage to the power ctrl register and switch the Power OFF
**************************************************************************/
bool set_card_voltage(uint8_t voltage, uint32_t device, uint32_t slot)
{
  return(did_host->set_voltage(voltage,device, slot));
}
/**************************************************************************
 *	bool card_power(bool bus_power_state)
 *	Used to switch ON/OFF the power to the card
**************************************************************************/
bool card_power(bool bus_power_state, uint32_t device, uint32_t slot)
{
  return(did_host->power(bus_power_state,device, slot));
}
/**************************************************************************
 *	bool init_host(uint32_t device, uint32_t slot)
 *	Initialise Host operating Voltage and Clock
**************************************************************************/
bool init_host(uint32_t device, uint32_t slot)
{
  return(did_host->host_init(device, slot));
}
/**************************************************************************
 *	bool set_card_clock(uint16_t freq_select, uint32_t device, uint32_t slot)
 *	set sd clk frequency
**************************************************************************/
bool set_card_clock(uint32_t device, uint32_t slot, uint32_t max_freq, uint32_t selection_mode)
{
  return(did_host->set_clock(device, slot , max_freq, selection_mode));
}
/**************************************************************************
 *	bool card_clock(bool clk_state,uint32_t device,uint32_t slot)
 *	Enable or disable sd clk
**************************************************************************/
bool card_clock(bool clk_state,uint32_t device,uint32_t slot)
{
  return(did_host->card_clk(clk_state, device, slot));
}
/**************************************************************************
 *	bool host_clock(bool clk_state,uint32_t device,uint32_t slot)
 *	Enable or disable sd clk
**************************************************************************/
bool host_clock(bool clk_state,uint32_t device,uint32_t slot)
{
  return(did_host->host_clk(clk_state, device, slot));
}
/**************************************************************************
 *	bool init_card(uint32_t device, uint32_t slot)
 *	Initilize card
**************************************************************************/
bool init_card(uint32_t device, uint32_t slot,bool voltage_switch, _init_status *status)
{
  return(did_host->card_init(device, slot,voltage_switch,status));
}
/**************************************************************************
 *	bool issue_cmd(_command *pcmd)
 *	Issue normal command to the card
**************************************************************************/
bool issue_command(_command *pcmd)
{
return(did_host->p_card->send_command(pcmd));
}
/**************************************************************************
 *	bool issue_cmd(_command *pcmd)
 *	Issue app command to the card
**************************************************************************/
bool issue_appcommand(_command *pcmd)
{
return(did_host->p_card->send_appcommand(pcmd));
}
/**************************************************************************
 *	bool get_card_status(_r1 *ptr_cardstatus,uint32_t device,uint32_t slot)
 *	get the status of the coard in the device and slot
**************************************************************************/
bool get_card_status( _r1 *ptr_cardstatus,uint32_t *error)
{
return(did_host->p_card->get_card_status(ptr_cardstatus,error));
}
/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool get_rca(uint16_t* ptr_rca,uint32_t *error)
{
return(did_host->p_card->get_rca(ptr_rca,error));
}
/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool get_all_cid(_r2 *ptr_cid,uint32_t *error)
{
return(did_host->p_card->get_all_cid(ptr_cid,error));
}
/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool go_idle(uint32_t *error)
{
return(did_host->p_card->go_idle(error));
}

/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool send_if_cond(uint8_t vhs,_r7* ptr_r7,uint32_t *error)
{
return(did_host->p_card->send_if_cond(vhs,ptr_r7,error));
}
/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool get_ocr( _r3* ptr_ocr,uint32_t *error)
{
return(did_host->get_ocr(ptr_ocr,error));
}
/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool send_op_cond(bool s18r,bool hcs,_r3* ptr_ocr,uint32_t *error)
{
return(did_host->p_card->send_op_cond(s18r,hcs,ptr_ocr,error));
}
/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool get_cid(_r2* ptr_cid,uint32_t *error)
{
return(did_host->p_card->get_cid( ptr_cid,error));
}

/**************************************************************************
 *	bool get_card_status(_uint16_t* ptr_rca,uint32_t device,uint32_t slot)
 *	get the rca of the card in the device and slot
**************************************************************************/
bool get_csd( _r2* ptr_csd,uint32_t *error)
{
return(did_host->get_csd( ptr_csd,error));
}
bool set_csd(uint32_t device, uint32_t slot,uint8_t *csd_data,bool speed, bool bus_width ,_r1 *status)
{
return (did_host->write_csd( device, slot, csd_data,speed, bus_width ,status));
}
bool set_csd_1_1_29(uint32_t device, uint32_t slot,uint8_t *csd_data,bool speed, bool bus_width ,_r1 *status)
{
return (did_host->write_csd_1_1_29( device, slot, csd_data,speed, bus_width ,status));
}
bool select_card( _r1 *status, uint32_t *error)
{
return(did_host->select_card( status, error));
}
bool deselect_card( _r1 *status,uint32_t *error)
{
return(did_host->p_card->deselect_card(status,error));
}
bool set_card_buswidth(uint8_t width,_r1 *status,uint32_t *error)
{
return(did_host->p_card->set_buswidth(width,status,error));
}
bool set_card_blocklength(uint32_t blocklength,_r1 *status,uint32_t *error)
{
return(did_host->p_card->set_blocklength(blocklength,status,error));
}

bool read_singleblock(uint32_t address,_r1 *status,uint32_t *error)
{
return(did_host->p_card->read_singleblock(address,status,error));
}

bool read_multiblock(uint32_t address,_r1 *status,uint32_t *error)
{
return(did_host->p_card->read_multiblock(address,status,error));
}

bool write_singleblock(uint32_t address,_r1 *status,uint32_t *error)
{
return(did_host->p_card->write_singleblock(address,status,error));
}

bool write_multiblock(uint32_t address,_r1 *status,uint32_t *error)
{
return(did_host->p_card->write_multiblock(address,status,error));
}

bool write_multiblock_error(uint32_t address,_r1 *status,uint32_t *error)
{
return(did_host->p_card->write_multiblock_error(address,status,error));
}

bool set_card_blockcount(uint32_t blockcount,_r1 *status,uint32_t *error)
{
return(did_host->p_card->set_blockcount(blockcount,status,error));
}

bool go_inactive(uint32_t *error)
{
return(did_host->go_inactive(error));
}

bool set_host_buswidth(bool buswidth, uint32_t device, uint32_t slot)
{
return(did_host->set_buswidth(buswidth, device, slot));
}

bool set_speed(bool speed, uint32_t device, uint32_t slot)
{
return(did_host->set_highspeed(speed, device, slot));
}

bool set_transfermode(bool mode, bool direction, uint8_t autocmd, bool block_count, bool dma, uint32_t device, uint32_t slot)
{
return(did_host->set_transfermode( mode,  direction,  autocmd,  block_count,  dma,  device,  slot));
}

bool set_host_block_length_n_count(uint16_t length, uint16_t count,uint32_t device, uint32_t slot)
{
return(did_host->set_block_length_n_count( length,  count, device, slot));
}

bool set_timeout_ctrl(uint8_t timeout,uint32_t device, uint32_t slot)
{
return(did_host->set_timeout_ctrl(timeout,device, slot));
}

bool transfer_complete( uint32_t *error)
{
return(did_host->transfer_complete(error, 0x10));
}
bool command_complete( uint32_t *error)
{
return(did_host->command_complete(error));
}
bool dma_complete( uint32_t *error)
{
return(did_host->dma_complete(error));
}




bool get_sd_status(uint32_t device , uint32_t slot , bool speed , bool width, _r1 *status , _sd_status *sd_status)
{
return(did_host->get_sd_status( device , slot , speed , width, status , sd_status));
}

bool int_status_enable(uint32_t device , uint32_t slot, uint32_t int_status)
{
return(did_host->int_status_enable( device ,  slot,  int_status));
}
bool int_signal_enable(uint32_t device , uint32_t slot, uint32_t int_status)
{
return(did_host->int_signal_enable( device ,  slot,  int_status));
}
bool force_event(uint32_t device , uint32_t slot, uint32_t force)
{
return(did_host->force_event( device ,  slot,  force));
}
uint32_t get_int_status(uint32_t device , uint32_t slot)
{
return(did_host->get_int_status( device , slot));
}
uint16_t get_autocmd_error(uint32_t device , uint32_t slot)
{
return(did_host->get_autocmd_error( device , slot));
}
bool init_data_transfer(uint32_t device, uint32_t slot ,bool speed, bool bus_width,uint16_t blockcount , uint32_t block_length )
{
return(did_host->init_data_transfer( device,  slot , speed,  bus_width, blockcount ,  block_length ));
}
bool erase_start_add(uint32_t address,_r1 *status, uint32_t *error)
{
return(did_host->p_card->erase_start_add(address,status,error)); 
}
bool erase_end_add(uint32_t address,_r1 *status,uint32_t *error)
{
return(did_host->p_card->erase_end_add(address,status,error)); 
}
bool erase(_r1 *status,uint32_t *error)
{
return(did_host->p_card->erase(status,error)); 
}
bool erase_cmd(_r1 *status,uint32_t *error)
{
return(did_host->p_card->erase(status,error)); 
}

bool erase_data(uint32_t start_address,uint32_t end_address,_r1 *status)
{
return ( did_host->erase_data(  start_address, end_address,status));
}
bool issue_switch_function(uint32_t device, uint32_t slot, bool speed,bool width,uint32_t argument,_r1 *status,uint8_t *ptr_buffer )
{
return ( did_host->issue_switch_function( device,  slot,  speed, width, argument,status, ptr_buffer ));
}
bool lock_unlock(uint32_t device, uint32_t slot,uint8_t *lock_card_data,bool speed, bool bus_width ,_r1 *status)
{
return ( did_host->lock_unlock( device,  slot,lock_card_data, speed,  bus_width , status));
}
bool get_scr(uint32_t device , uint32_t slot, _scr *ptr_scr,_r1 *status,bool speed, bool width)
{
return ( did_host->get_scr( device ,  slot, ptr_scr,status, speed,  width));
}
bool get_supported_functions(uint32_t device,uint32_t slot, bool speed,bool width,_function *func, _r1 *status)
{
return ( did_host->get_supported_functions( device, slot,  speed, width,func, status));
}
bool switch_function(uint32_t device , uint32_t slot, bool speed,bool width,_function *func, _r1 *status)
{
return ( did_host->switch_function( device , slot,  speed, width,func, status));
}
bool sdma_write(uint32_t device , uint32_t slot , uint32_t card_address , uint32_t *phy_address , uint32_t mem_size, uint32_t blk_size, uint32_t blk_count, bool speed, bool width, uint8_t autocmd)
{
return( did_host->sdma_write(device , slot , card_address , phy_address , mem_size, blk_size, blk_count, speed, width, autocmd));
}
bool sdma_read(uint32_t device , uint32_t slot , uint32_t card_address , uint32_t *phy_address , uint32_t mem_size, uint32_t blk_size, uint32_t blk_count, bool speed, bool width, uint8_t autocmd,sdma_red_datatype *datastruct)
{
return( did_host->sdma_read(device , slot , card_address , phy_address , mem_size, blk_size, blk_count, speed, width, autocmd , datastruct));
}

bool pio_write(uint32_t device, uint32_t slot,bool speed, bool bus_width,uint8_t autocmd , uint32_t card_address,uint8_t *ptr_buffer,uint32_t blk_size,uint32_t blk_count)
{
return( did_host->pio_write( device,  slot, speed,  bus_width, autocmd ,  card_address, ptr_buffer, blk_size, blk_count,false));
}
bool pio_write_with_block_gap(uint32_t device, uint32_t slot,bool speed, bool bus_width,uint8_t autocmd , uint32_t card_address,uint8_t *ptr_buffer,uint32_t blk_size,uint32_t blk_count)
{
return( did_host->pio_write( device,  slot, speed,  bus_width, autocmd ,  card_address, ptr_buffer, blk_size, blk_count,true));
}

bool pio_read_with_block_gap(uint32_t device, uint32_t slot,bool speed, bool bus_width,uint8_t autocmd , uint32_t card_address,uint8_t *ptr_buffer,uint32_t blk_size,uint32_t blk_count)
{
return( did_host->pio_read( device,  slot, speed,  bus_width, autocmd ,  card_address, ptr_buffer, blk_size, blk_count,true));
}
bool dma2_write_with_asynchabort(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_select)
{
return( did_host->dma2_write( device ,  slot ,  card_address , dma_table ,  blk_size,  blk_count,  speed,  width,  autocmd,  dma_select, false,true));
}

bool pio_read(uint32_t device, uint32_t slot,bool speed, bool bus_width,uint8_t autocmd , uint32_t card_address,uint8_t *ptr_buffer,uint32_t blk_size,uint32_t blk_count)
{
return( did_host->pio_read( device,  slot, speed,  bus_width, autocmd ,  card_address, ptr_buffer, blk_size, blk_count,false));
}
bool dma2_write(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_select)
{
return( did_host->dma2_write( device ,  slot ,  card_address , dma_table ,  blk_size,  blk_count,  speed,  width,  autocmd,  dma_select, false,false));
}
bool dma2_write_3_4_16(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_select,uint8_t* LockData)
{
return( did_host->dma2_write_3_4_16( device ,  slot ,  card_address , dma_table ,  blk_size,  blk_count,  speed,  width,  autocmd,  dma_select, false,false,LockData));
}

bool dma2_write_with_block_gap(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_select)
{
return( did_host->dma2_write( device ,  slot ,  card_address , dma_table , blk_size, blk_count, speed, width, autocmd, dma_select, true,false));
}
bool dma2_read(uint32_t device , uint32_t slot , uint32_t card_address , uint64_t dma_table , uint32_t blk_size, uint64_t blk_count, bool speed, bool width, uint8_t autocmd, uint8_t dma_sel)
{
return( did_host->dma2_read( device ,  slot ,  card_address , dma_table ,  blk_size,  blk_count,  speed,  width,  autocmd,  dma_sel));
}
bool set_normal_mode(uint32_t device, uint32_t slot, uint16_t speed_mode, uint16_t clock_selection_mode)
{
return( did_host->set_normal_mode(device, slot, speed_mode, clock_selection_mode));
}
bool set_UHS_mode(uint32_t device, uint32_t slot, uint16_t speed_mode, uint16_t clock_selection_mode)
{
return( did_host->set_UHS_mode(device, slot, speed_mode, clock_selection_mode));
}

bool tuning_procedure(uint32_t device,uint32_t slot)
{
return( did_host->issue_tuning( device, slot));
}

bool error_interrupt_recovery(uint32_t device, uint32_t slot , uint32_t error)
{
return( did_host->error_interrupt_recovery( device,  slot, error));
}
/**********************************************************/



#if SD_PCI
/**********************************************************/
void get_bus_device_function(uint32_t device , uint16_t *bus_number , uint16_t *device_number , uint16_t *function_number)
{
	did_host->get_bus_device_function( device , bus_number , device_number , function_number);
}
/**********************************************************/
#endif


/**********************************************************/
bool recover_normal_error(uint32_t device , uint32_t slot)
{
return( did_host->recover_normal_error( device,  slot));
}
bool datatransfer_error_inject(uint32_t device, uint32_t slot,bool speed , uint32_t error_type,bool data_transfer_direction, uint32_t *error)
{
return( did_host->datatransfer_error_inject(device, slot, speed , error_type, data_transfer_direction, error));
}
bool program_csd(_r1 *status,uint32_t *error)
{
return(did_host->p_card->program_csd(status,error)); 
}

//SDIO Read/Write 
bool direct_io_read(uint32_t function,uint32_t reg_offset, _r5 *status,uint32_t *error)
{
return (did_host->direct_io_read(function,reg_offset, status,error));
}
bool direct_io_write(bool raw , uint32_t function,uint32_t reg_offset, uint32_t data ,_r5 *status,uint32_t *error)
{
return (did_host->direct_io_write(raw,function,reg_offset,data, status,error));
}
bool extended_io_byte_read(uint32_t device, uint32_t slot , uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address,bool block_mode, uint32_t count, uint8_t *ptr_buffer, _r5 *status,uint32_t *error)
{
return (did_host->extended_io_read( device,  slot , function, reg_offset, fixe_or_inc_address, block_mode, count, ptr_buffer, status,error));
}
bool extended_io_byte_write(uint32_t device, uint32_t slot ,uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address,bool block_mode, uint32_t count, uint8_t *ptr_buffer,_r5 *status,uint32_t *error)
{
return (did_host->extended_io_write(device,  slot ,function, reg_offset, fixe_or_inc_address,block_mode, count,ptr_buffer, status,error));
}
#if 0
bool extended_io_block_read(uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address, uint32_t count, uint8_t *ptr_buffer,_r5 *status,uint32_t *error)
{
return (did_host->extended_io_read(function, reg_offset, fixe_or_inc_address,true, count,ptr_buffer, status,error));
}
bool extended_io_block_write(uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address, uint32_t count, uint8_t *ptr_buffer,_r5 *status,uint32_t *error)
{
return (did_host->extended_io_write(function, reg_offset, fixe_or_inc_address,true, count, ptr_buffer,status,error));
}
#endif

bool set_clock(uint32_t device, uint32_t slot, uint32_t Max_freq, uint32_t clock_selection_mode)
{
return (did_host->set_clock( device,  slot,  Max_freq, clock_selection_mode));
}
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/
/************************************************************************************************************************************************/


#if 0




#endif
/************************** API TO EXPORT END********************************/
