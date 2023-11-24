#include "sd_wrapper.h"
#include <env_time.h>
#include <env_device_interface.h>
 #include "sd3_0.h"

using namespace env;
coreutil::Logger SDWrLogger::mLogger("SDWLOG");

#if SD_PCI
 #include <env_pci.h>
//static env::System system1;
env::PciClient* client1;

uint32_t BUS 		= 0x00;
uint32_t DEVICE  	= 0x14;
uint32_t FUNCTION 	= 0x07;
bool create_pci_device(uint32_t pci_bus, uint32_t pci_device, uint32_t pci_function)
{
	uint32_t            pci_domain = 0 ;
	//altered the way env::System() is called.
	env_device_interface env_dev;
	env::System      *pDevice; 
	pDevice = env_dev.get_env_system();
	env::System* m_env_system1 = pDevice;
	 BUS 		= pci_bus;
	 DEVICE  	= pci_device;
	 FUNCTION 	= pci_function;
	bool result = true;
    try
    {
	    env::System &env_device = *(env_device_interface().get_env_system());
        env::PciLocation location(pci_domain, pci_bus, pci_device, pci_function) ;
        client1 = new env::PciClient(*env_device.pci().device(location)) ;
#ifdef OLD_WAY_ENV_SYSTEM
		env::PciLocation location(pci_domain, pci_bus, pci_device, pci_function) ;
		//client1 = new env::PciClient(*env_device.pci().device(location)) ;
		 env::PciDevice* my_pcip ;
		 if (pDevice!=NULL) {
			//printf("pDevice not NULL \n");
            PciEnumerator *pciEnum = pDevice->get_pci();
			//pciEnum = &pDevice->pci();
			//pciEnum = pDevice->&pci();
			
			if(pciEnum) {
				//printf("pciEnum not null\n");
				env::PciDevice* pPciDevice = pciEnum->device(location);
				if (pPciDevice != NULL) {
				client1 = new env::PciClient(*pDevice->pci().device(location)) ; //pDevice not needed
				//printf("pPciDevice is not NULL\n");
				
				}
				else {
					//printf("pPciDevice NULL\n");
				}
			}
			else {
				//printf("pciEnum null\n");
			}
		}
		else {
			//printf("pDevice is null\n");
		}
#endif /* OLD_WAY_ENV_SYSTEM */		
		if (client1 == NULL) {
			SD_WLOG_DEBUG()<< "client1 is NULL"<<std::endl;
			exit(0);
		}
		SD_WLOG_DEBUG()<<"In create_pci_device client1"<<std::hex<<client1<<std::endl;
	}

    catch (std::runtime_error& e)
    {
        SD_WLOG_DEBUG()<<"run time exception"<<std::endl;
		//printf("runtime exception\n");
        std::cerr << e.what() << std::endl ;
		result = false;
		exit(0);
    }	
	return result;
}
#endif






#if SD_MMIO
	extern const uint64_t SdCfgBaseAddrss ;
	extern const uint64_t SdHostcntrlBaseAddrss ;
	
	// UARTs virtual addresses
	uintptr_t	SdCfgVirtAddrss;
	uintptr_t	SdHostcntrlVirtAddrss;
		
	

	/***********************************************************/	
	uint64_t sd_hd_read_mmio(size_t offset,unsigned short width)
	{
		uint64_t value = 0x00;
				
		uint32_t cfg_or_hostcntrl = 0;  // or cfg space and 1 for host cntroller register
		value = SdMMIORegRead(cfg_or_hostcntrl, offset, width);
		return ((uint64_t)value);
			
		return 0x00;
	}
	/***********************************************************/	







	/***********************************************************/
	bool Enable_IO_Mux_AM4_ZP_RV_SD_MMIO()
	{
		const uint64_t SdIomuxBaseAddrss = 0xFED80000; //0xFED80D00;  
		uint64_t SdIomuxVirtAddrss;
		
		PSD_INFO pDev_Iomux = new SD_INFO;
		uint64_t value = 0;
		
		env::Map *MapSrcIomux = NULL;
		env::System *my_sys_iomux = NULL;
		
		bool status = true;
		
		/* actully 256 bytes required for IOmux pins, but lets allocate 4096, and then move to IOMUX location*/
		MapSrcIomux = my_sys_iomux->pmap((void *)SdIomuxBaseAddrss, 4096);
		SdIomuxVirtAddrss = (uint64_t)MapSrcIomux->getVirtualAddress();
		if(SdIomuxVirtAddrss == 0)
		{
			SD_WLOG_DEBUG() << "virtual memory allocation failed for SD pins IOMUX MMIO space " <<std::endl;
			status =false;
			return status;
		}
		
		//Add, 0xD00 to SdIomuxVirtAddrss, to reach SD CMD pins based address
		SdIomuxVirtAddrss = SdIomuxVirtAddrss + 0xD00; 
		pDev_Iomux->pRegMap = (void*)(SdIomuxVirtAddrss);
		
		/*********************************************************************************/		
		//## IOMUXx15     LPC_PD_L_SD_CMD_AGPIO21
		uint8_t *gpio21   = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x15;
		value = READ_UBYTE(gpio21);  //Read
		//SD_WLOG_DEBUG() << "Before modifying, GPIO 21 value :: \n" << std::hex<< value <<std::endl;
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio21, value);  		//Write back modified value
		value = READ_UBYTE(gpio21);  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_CMD pin @ GPIO 21 value :: " << std::hex<< value <<std::endl;
		
		
		//## IOMUXx46     EGPIO70_SD_CLK
		uint8_t *gpio70    = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x46;
		value = READ_UBYTE(gpio70 );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio70 , value);  		//Write back modified value
		value = READ_UBYTE(gpio70);  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_CLK pin @GPIO 71 value :: " << std::hex<< value <<std::endl;
		
		
		
		//## IOMUXx68  LAD0/SD_DAT0/EGPIO104            
		uint8_t *gpio104    = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x68;
		value = READ_UBYTE(gpio104 );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio104 , value);  		//Write back modified value
		value = READ_UBYTE(gpio104);  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA0 pin @GPIO 104 value :: " << std::hex<< value <<std::endl;
		
		
		//## IOMUXx69  LAD1/SD_DAT1/EGPIO105                       
		uint8_t *gpio105    = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x69;
		value = READ_UBYTE(gpio105 );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio105 , value);  		//Write back modified value
		value = READ_UBYTE(gpio105);  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA1 pin @GPIO 105 value :: " << std::hex<< value <<std::endl;
		
		
		//## IOMUXx6A  LAD2/SD_DAT2/EGPIO106                             
		uint8_t *gpio106    = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x6A;
		value = READ_UBYTE(gpio106);  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio106 , value);  		//Write back modified value
		value = READ_UBYTE(gpio106);  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA2 pin @GPIO 106 value :: " << std::hex<< value <<std::endl;
		
		//## IOMUXx6B  LAD3/SD_DAT3/EGPIO107                               
		uint8_t *gpio107    = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x6B;
		value = READ_UBYTE(gpio107);  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio107 , value);  		//Write back modified value
		value = READ_UBYTE(gpio107);  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA3 pin @GPIO 107 value :: " << std::hex<< value <<std::endl;
		
		
		/* //## IOMUXx4A  LPCCLK0/SD_DAT4/EGPIO74                                  
		uint8_t *gpio74     = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x4A;
		value = READ_UBYTE(gpio74 );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio74  , value);  		//Write back modified value
		value = READ_UBYTE(gpio74 );  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA4 pin @GPIO  74 value :: " << std::hex<< value <<std::endl;
		
		
		//## IOMUXx58  LPC_CLKRUN_L/SD_DAT5/AGPIO88                                    
		uint8_t *gpio88      = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x58;
		value = READ_UBYTE(gpio88  );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio88   , value);  		//Write back modified value
		value = READ_UBYTE(gpio88  );  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA5 pin @GPIO 88 value :: " << std::hex<< value <<std::endl;
		
		
		
		//## IOMUXx4B  LPCCLK1/SD_DAT6/EGPIO75                                           
		uint8_t *gpio75       = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x4B;
		value = READ_UBYTE(gpio75   );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio75    , value);  		//Write back modified value
		value = READ_UBYTE(gpio75 );  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA6 pin @GPIO 75 value :: " << std::hex<< value <<std::endl;
		
		
		//## IOMUXx57  SERIRQ/SD_DAT7/AGPIO87                                                      
		uint8_t *gpio87        = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x57;
		value = READ_UBYTE(gpio87    );  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio87     , value);  		//Write back modified value
		value = READ_UBYTE(gpio87  );  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DATA7 pin @GPIO 87 value :: " << std::hex<< value <<std::endl;
		
		
		//## IOMUXx6D  LFRAME_L/SD_DS/EGPIO109                                                           
		uint8_t *gpio109         = ((uint8_t *)pDev_Iomux->pRegMap)+ (uint8_t)0x6D;
		value = READ_UBYTE(gpio109);  //Read
		value |= 0x01;//  (1 << 0);			//modify
		WRITE_UBYTE(gpio109, value);  		//Write back modified value
		value = READ_UBYTE(gpio109   );  //Read it again
		SD_WLOG_DEBUG() << "After modifying, SD_DS pin @GPIO 109 value :: " << std::hex<< value <<std::endl; */
		
		
		
		
		//number |= 1 << x;
		return true;
	}
	/*************************************************************************/
	
	
	
	/************************************************************************/
	// get virtual address for selected SD
	bool SdGetVirtualAdress(void)
	{
		 //const uint64_t SdCfgBaseAddrss = 0xFEDD5800;  
		 const uint64_t SdHostcntrlBaseAddrss = 0xFEDD5000;
		 
				 
		env::Map *MapSrc = NULL;
		env::System *my_sys = NULL;
		bool status = true;
		
			
		//getting virtual addresss for SD host controller space
		if (SdHostcntrlBaseAddrss == 0x0)
		{
			SD_WLOG_DEBUG() << "Not a valid base address for SD Host Space in selected SOC" <<std::endl;
			status =false;
			return status;
		}
		else
		{
			MapSrc = my_sys->pmap((void *)SdHostcntrlBaseAddrss, 4096);
			SdHostcntrlVirtAddrss = (uint64_t)MapSrc->getVirtualAddress();
			if(SdHostcntrlVirtAddrss == 0)
			{
				SD_WLOG_DEBUG() << "virtual memory allocation failed for SD HOST CNTRLER" <<std::endl;
				status =false;
				return status;
			}
		}
		
		//getting virtual addresss for CFG space too, by adding offset of 2K
		SdCfgVirtAddrss = SdHostcntrlVirtAddrss + (uint64_t)0x800;
		 
		 //getting virtual addresss for CFG space
		/* if (SdCfgBaseAddrss == 0x0)
		{
			SD_WLOG_DEBUG() << "Not a valid base address for SD CONFIG in selected SOC" <<std::endl;
			status =false;
			return status;
		}
		else
		{
			//SD_CFG_PAGE_SIZE ====> 2048
			MapSrc = my_sys->pmap((void *)SdCfgBaseAddrss, SD_CFG_PAGE_SIZE);
			SdCfgVirtAddrss = (uint64_t)MapSrc->getVirtualAddress();
			if(SdCfgVirtAddrss == 0)
			{
				SD_WLOG_DEBUG() << "virtual memory allocation failed for SD CFG space \n" <<std::endl;
				status =false;
				return status;
			}
		} */
		
		 return status;
	}
	/************************************************************************/
	
	
	
	
	
	//read SD space of SD config and host controller register space registers
	
	/*--------------------------------------------------------------------------
	  Read from Memory Mapped Registers

		Inputs:

		pDev    - pointer to module with device information
		offset     - register offset
		nbytes  - number of bytes to read from the address
					  (can't exceed 4, since value is 32 bits)

		Outputs:

		value   - double word value read from the MM register

		Description:

		This routine reads from memory mapped register given it's offset.
		The base address is read from the device information module.

		The function performs only byte reads.

	--------------------------------------------------------------------------*/
	/*************************************************************/
	uint64_t  SdMMIORegRead( uint32_t cfg_or_hostcntrl, size_t offset, uint16_t nbytes)
	{
		PSD_INFO pDev = new SD_INFO;
		uint64_t value = 0;
		
		if (cfg_or_hostcntrl == 0) // read SD cfg space regisers
		{
			
			pDev->pRegMap = (void*)(SdCfgVirtAddrss);
		}
		else if (cfg_or_hostcntrl == 1) // read SD host controller regisers
		{
			pDev->pRegMap = (void*)(SdHostcntrlVirtAddrss);
		}
		
		uint8_t *pIo   = ((uint8_t *)pDev->pRegMap)+ (uint8_t)offset;

		switch(nbytes)
		{
			case 8:
		  case 4:
			value = READ_ULONG(pIo);
			break;

		  case 3:
			value = READ_ULONG(pIo);
			value &= 0x00FFFFFF;
			break;

		  case 2:
			value = READ_UWORD(pIo);
			break;

		  case 1:
			value = READ_UBYTE(pIo);
			break;

		  default:
			SD_WLOG_DEBUG()<<"deafult case of SdMMIORegRead ---> "<<std::endl;
			SD_WLOG_DEBUG()<<"no of bytes trying to read is:  "<<std::dec << nbytes<<std::endl;
			//printf("should not reach here, check program sequence.\n");
			break;
		}
		 return value;
	  
	}
	/**********************************************************/
	
	
	
	
	/**********************************************************/
	void sd_hd_write_mmio(size_t offset,size_t value,unsigned short width)
	{
		
		uint32_t cfg_or_hostcntrl = 0;  // 0 for cfg space and 1 for host cntroller register
		SdMMIORegWrite(cfg_or_hostcntrl, offset, value, width);
		
	}

	
	/*--------------------------------------------------------------------------
	  Write to Memory Mapped Registers
	 
		Inputs:

		pDev    - pointer to module with device information
		offset     - register offset
		value   - value to be written to MM register
		nbytes  - number of bytes in the register (1-4)

		Outputs:

		none.

		Description:

		This routine writes to memory mapped register given it's offset.
		The base address is read from the device information module.

		This function does byte writes.

	--------------------------------------------------------------------------*/
	void  SdMMIORegWrite(uint32_t cfg_or_hostcntrl, size_t offset, size_t value, uint16_t nbytes)
	{
		PSD_INFO pDev = new SD_INFO;
		
		// get and assign the virtual address of cfg_or_hostcntrl
		if (cfg_or_hostcntrl == 0) // read SD cfg space regisers
		{
			pDev->pRegMap = (void*)(SdCfgVirtAddrss);
		}
		else if (cfg_or_hostcntrl == 1) // read SD host controller regisers
		{
			pDev->pRegMap = (void*)(SdHostcntrlVirtAddrss);
		}
				
				
		uint8_t *pIo = ((uint8_t *)pDev->pRegMap) + (uint8_t)offset;

		switch(nbytes)
		{
			case 8:
		  case 4:
			WRITE_ULONG(pIo, value);
			
			break;

		  case 3:
			WRITE_ULONG(pIo, (value & 0x00FFFFFF) | 0xFF000000);
			break;

		  case 2:
			WRITE_UWORD(pIo, value & 0xffff);
			break;

		  case 1:
			WRITE_UBYTE(pIo, value & 0xff);
			break;

		  default:
			SD_WLOG_DEBUG()<<"deafult case of SdMMIORegWrite ---> "<<std::endl;
			SD_WLOG_DEBUG()<<"no of bytes trying to write is:  "<<std::dec << nbytes<<std::endl;
			//printf("Should not happened, check program sequence!\n");
			break;
		}
		return;
	}
	
#endif

	/*****************************************************************/
	void SET_SD_REGISTER_READ(_reg *x,uint32_t y,uint32_t t_device,uint32_t t_slot)
	{
		x->sd_reg.device = t_device; 
		x->sd_reg.slot   = t_slot; 
		x->sd_reg.offset = sd_register[y].offset; 
		x->sd_reg.width  = sd_register[y].width; 
		x->sd_reg.value  = 0x0000000000000000;
	}
	/*****************************************************************/



	/*****************************************************************/																 
	void SET_SD_REGISTER_WRITE(_reg *x,uint32_t y,uint32_t t_device,uint32_t t_slot,uint32_t t_value)
	{
		 x->sd_reg.device = t_device; 
		 x->sd_reg.slot   = t_slot; 
		 x->sd_reg.offset = sd_register[y].offset; 
		 x->sd_reg.width  = sd_register[y].width; 
		 x->sd_reg.value  = t_value;
	}
	/*****************************************************************/




	/*****************************************************************/
	bool rhost_reg(_reg *preg)
	{
	  //char *name		 	= sd_register[host_offset].name;
	  //size_t offset  		= preg->sd_reg.offset;
	
		preg->sd_reg.value = (uint64_t)(sd_read_reg((size_t)(preg->sd_reg.offset),preg->sd_reg.width));
	  return true;
	}
	/*****************************************************************/




	/*****************************************************************/
	bool whost_reg(_reg *preg)
	{
	 // char* reg_name = sd_register[host_offset].name;
	  size_t offset  = preg->sd_reg.offset;
	  
	  sd_write_reg(offset, preg->sd_reg.value, preg->sd_reg.width);
	  return true;
	}
	/*****************************************************************/
			
	
	
	
	

	/***************************************************************************************************************/
	/************************************************** ENV Lib Access *********************************************/
	/***************************************************************************************************************/
	uint32_t port_read(uint8_t value)
	{
		uint32_t reg_value = 0x00,temp_reg = 0x00;
		uint8_t byte_count =0x00;
		for(byte_count = 0 ; byte_count < 4 ; byte_count++)
		{
		  env::System::portw8(0xcd6, (value + byte_count));
		  temp_reg  = (uint32_t)env::System::portr8(0xcd7);
		  reg_value  = (reg_value | (temp_reg << (byte_count*8)));
		}
		return reg_value;
	}
	/********************************************************************************************/
	
	
	
	#if SD_PCI
	/********************************************************************************************/
	int64_t hd_read_pci(unsigned short bus1,unsigned short device1,unsigned short function1,size_t offset,unsigned short width)
	{
		uint64_t value = 0x00;
		if (client1 == NULL) {
			//printf ("client1 NULL returning...\n");
			return value;
		}
		if((BUS == bus1) && (DEVICE == device1) && (FUNCTION == function1))
		{
			switch(width)
			{
			case 1: 
				value = (uint8_t)client1->cfgr8(offset);
				
				//printf("PCI Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint8_t)value);
			case 2:
				value = (uint16_t)client1->cfgr16(offset);
				
				//printf("PCI Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint16_t)value);
			case 4:
				value = (uint32_t)client1->cfgr32(offset);
				
				//printf("PCI Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint32_t)value);
			case 8:
				value = (uint64_t)client1->cfgr64(offset);
				
				//printf("PCI Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint64_t)value);
			}
		}
		return 0x00;
	}
	/******************************************************************************/





	/****************************************************************************/
	void hd_write_pci(unsigned short bus1,unsigned short device1,unsigned short function1,size_t offset,size_t value,unsigned short width)
	{

		if((BUS == bus1) && (DEVICE == device1) && (FUNCTION == function1))
		{


			try{
				switch(width)
				{
				case 1: 
					(client1->cfgw8(offset,value));
					break;
				case 2:
					(client1->cfgw16(offset,value));
					break;
				case 4:
					(client1->cfgw32(offset,value));
					break;
				case 8:
					(client1->cfgw64(offset,value));
					break;
				}
				
				//printf("PCI Register Write - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);	
			}
			catch(std::runtime_error& e)
			{
			std::cerr << "Exception for Bus :"<< "--> "<<e.what() << std::endl ;
			}
		}
	}
	#endif
	/********************************************************************************************/


	


	/********************************************************************************************/
	uint64_t sd_read_reg(size_t offset,unsigned short width)
	{
		uint64_t value = 0x00;
		//uint8_t x =0;
		#if SD_PCI
		try{
			switch(width)
			{
			case 1: 
				value = (uint8_t)client1->regr8(offset);
				//if((offset ==0x20) || (offset ==0x21) || (offset ==0x22) || (offset ==0x23))
				//	printf("\n sd_read_reg: value: %x x : %x\n", (uint8_t)value,x);
				//if(offset != 0x20 && offset != 0x21 && offset != 0x22 && offset != 0x23)
					//printf("Host Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint8_t)value);
			case 2:
				value = (uint16_t)client1->regr16(offset);
				//if(offset != 0x20 && offset != 0x21 && offset != 0x22 && offset != 0x23)
				//printf("Host Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint16_t)value);
			case 4:
				value = (uint32_t)client1->regr32(offset);
				//if(offset != 0x20 && offset != 0x21 && offset != 0x22 && offset != 0x23)
				//printf("Host Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint32_t)value);
			case 8:
				value = (uint64_t)client1->regr64(offset);
				//if(offset != 0x20 && offset != 0x21 && offset != 0x22 && offset != 0x23)
				//printf("Host Register Read  - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);
				return ((uint64_t)value);
			}
		}	
			catch(std::runtime_error& e)
			{
				std::cerr << "Exception for Bus :"<< "--> "<<e.what() << std::endl ;
			}
		#elif SD_MMIO	// read MMIO space registers
		
		//SD_WLOG_DEBUG()<<"in middle MMIO SPace..... ---> "<<std::endl;
			uint32_t cfg_or_hostcntrl = 1;  // or 0 = cfg space and 1 for host cntroller register
			value = SdMMIORegRead(cfg_or_hostcntrl, offset, width);
			//SD_WLOG_DEBUG()<<"In sd_read_reg  preset register value ---> "<<std::hex<<value<<std::endl;
			
			return ((uint64_t)value);
			
		#endif
		//SD_WLOG_DEBUG()<<"out side..... ---> "<<std::endl;
		
		return 0x00;
	}

	void sd_write_reg(size_t offset,uint64_t value,unsigned short width)
	{
		#if SD_PCI
		switch(width)
		{
		case 1: 
			client1->regw8(offset,value);
			//if((offset ==0x20) || (offset ==0x21) || (offset ==0x22) || (offset ==0x23))
			//	printf("\n sd_write_reg : value: %x\n", value);
			break;
		case 2:
			client1->regw16(offset,value);
			break;
		case 4:
			client1->regw32(offset,value);
			break;
		case 8:
			client1->regw64(offset,value);
			break;
		}
		
		//if(offset != 0x20 && offset != 0x21 && offset != 0x22 && offset != 0x23)
		//printf("Host Register Write - Offset -> 0x%02X, Width -> 0x%02X, Value -> 0x%X\n",offset,width,value);	
		#elif SD_MMIO
				uint32_t cfg_or_hostcntrl = 1;  // or cfg space and 1 for host cntroller register
			 SdMMIORegWrite(cfg_or_hostcntrl, offset, value, width);
			
		#endif
		
	}

