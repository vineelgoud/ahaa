
#if SD_PCI
#include "sd_pci.h"
#include <env_time.h>
#include <stdio.h>

    _pci_node::_pci_node()
	{
		pci_header.vendor_id	= 0xff;
		pci_header.device_id	= 0xff;
		reg.pci_or_mmio_reg.offset		= 0x00;
		reg.pci_or_mmio_reg.value		= 0x00;
		reg.pci_or_mmio_reg.width   	= 0x00;
		bus						= 0x00;
		device					= 0x00;
		function				= 0x00;
		pci_header.vendor_id	= 0x00;
		pci_header.device_id	= 0x00;
		pci_header.command		= 0x00;	
		pci_header.status	 	= 0x00;
		pci_header.revision_id	= 0x00;
		pci_header.classcode 	= 0x00;
		pci_header.cachelinesize= 0x00;
		pci_header.latencytimer	= 0x00;
		pci_header.headertype	= 0x00;
		pci_header.bist			= 0x00;
		pci_header.bar0			= 0x00;
		pci_header.bar1			= 0x00;
		pci_header.bar2			= 0x00;
		pci_header.bar3			= 0x00;
		pci_header.bar4			= 0x00;
		pci_header.bar5			= 0x00;
		pci_header.slotinfo		= 0x00;	
		configured				= false;
	}

	_pci_node::~_pci_node()
	{
		/* Do nothing now */
	}

	bool _pci_node::read_pci()
	{
	bool result = false;
		if(configured != false)
		{
		     reg.pci_or_mmio_reg.value = hd_read_pci(PCI_LOCATION,reg.pci_or_mmio_reg.offset, reg.pci_or_mmio_reg.width);
			 result = true;
		}
	 return result;
	}

	bool _pci_node::write_pci()
	{
	bool result = false;
		if(configured != false)
		{
			hd_write_pci(PCI_LOCATION,reg.pci_or_mmio_reg.offset,reg.pci_or_mmio_reg.value,reg.pci_or_mmio_reg.width);
			result = true;
		}
	return result;
	}

	bool _pci_node::get_header(_pci_type0_header *header)
	{
	bool result = false;
		if(configured == true)
		{
		header->vendor_id			= pci_header.vendor_id		 ; 
		header->device_id	        = pci_header.device_id	     ; 
		header->command		        = pci_header.command		 ; 
		header->status	 	        = pci_header.status	 	     ; 
		header->revision_id	        = pci_header.revision_id	 ; 
		header->classcode 	        = pci_header.classcode 	     ; 
		header->cachelinesize       = pci_header.cachelinesize   ; 
		header->latencytimer	    = pci_header.latencytimer	 ; 
		header->headertype	        = pci_header.headertype	     ; 
		header->bist			    = pci_header.bist			 ; 
		header->bar0			    = pci_header.bar0			 ; 
		header->bar1			    = pci_header.bar1			 ; 
		header->bar2			    = pci_header.bar2			 ; 
		header->bar3			    = pci_header.bar3			 ; 
		header->bar4			    = pci_header.bar4			 ; 
		header->bar5			    = pci_header.bar5			 ; 
		header->slotinfo		    = pci_header.slotinfo		 ; 
		header->bus					= bus;
		header->device    			= device;
		header->function			= function;
		result = true;
		}
		return result;
	}
void _pci_node::get_bus_device_function(uint16_t *bus_number,uint16_t *device_number , uint16_t *function_number)
{
*bus_number			=	bus;
*device_number		=	device;
*function_number	=	function;

}
	bool _pci_node::init_node(uint8_t mbus, uint8_t mdevice, uint8_t mfunction)
	{
		
		bus		   			= mbus;
		device	   			= mdevice;
		function   			= mfunction;
		configured 			= true;
		
		pci_header.vendor_id      = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_VENDORID			),(unsigned short)(2));
	    pci_header.device_id      = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_DEVICEID			),(unsigned short)(2));
	    pci_header.command        = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_OR_MMIO_COMMAND		    ),(unsigned short)(2));
	    pci_header.status         = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_STATUS		 	),(unsigned short)(2));
	    pci_header.revision_id    = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_REVISION_ID	    ),(unsigned short)(1));
	    pci_header.classcode      = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_CLASS_CODE	    ),(unsigned short)(4));
	    pci_header.cachelinesize  = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_CACHELINESIZE 	),(unsigned short)(1));
	    pci_header.latencytimer   = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_LATENCYTIMER 	),(unsigned short)(1));
	    pci_header.headertype     = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_BIST				),(unsigned short)(1));
	    pci_header.slotinfo       = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_SLOTINFO        	),(unsigned short)(1));
	    pci_header.bist           = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_BASEADDRESS	    ),(unsigned short)(1));

		size_t bar_first  	= GET_BAR(pci_header.slotinfo);
		size_t last_bar   	= bar_first + (PCI_TOTAL_SLOT(pci_header.slotinfo) * PCI_BAR_WIDTH);
		
		if((last_bar > SDHC_PCI_BAR5) || (last_bar < SDHC_PCI_BAR0) || (bar_first > SDHC_PCI_BAR5) || (bar_first < SDHC_PCI_BAR0))
		{ 
			std::cout << "PCI BAR address error" << std::endl;
			return false;
		}
		switch(PCI_TOTAL_SLOT(pci_header.slotinfo))
		{
			case 5:
			  pci_header.bar5 = hd_read_pci(PCI_LOCATION,(size_t)(last_bar), (unsigned short)(PCI_BAR_WIDTH));
			  last_bar -= PCI_BAR_WIDTH;	                                 
  		    case 4:                                                          
			  pci_header.bar4 = hd_read_pci(PCI_LOCATION,(size_t)(last_bar), (unsigned short)(PCI_BAR_WIDTH));
			  last_bar -= PCI_BAR_WIDTH;                                     
			case 3:                                                          
			  pci_header.bar3 = hd_read_pci(PCI_LOCATION,(size_t)(last_bar), (unsigned short)(PCI_BAR_WIDTH));
			  last_bar -= PCI_BAR_WIDTH;                                     
			case 2:                                                          
			  pci_header.bar2 = hd_read_pci(PCI_LOCATION,(size_t)(last_bar), (unsigned short)(PCI_BAR_WIDTH));
			  last_bar -= PCI_BAR_WIDTH;                                     
			case 1:                                                          
			  pci_header.bar1 = hd_read_pci(PCI_LOCATION,(size_t)(last_bar), (unsigned short)(PCI_BAR_WIDTH));
			  last_bar -= PCI_BAR_WIDTH;                                     
			case 0:                                                          
			  pci_header.bar0 = hd_read_pci(PCI_LOCATION,(size_t)(last_bar), (unsigned short)(PCI_BAR_WIDTH));
		}

	/*Enable Memory Space Access and Bus Master*/

	hd_write_pci(PCI_LOCATION,(size_t)(SDHC_PCI_OR_MMIO_COMMAND),(pci_header.command | PCI_ENABLE_MEMACCESS_BUSMASTER),(unsigned short)(2));
	//pci_header.command = hd_read_pci(PCI_LOCATION,(size_t)(SDHC_PCI_OR_MMIO_COMMAND),(unsigned short)(2));	
	return true;
	}

	_pci_handler::_pci_handler()
	{
		total_pci		 	= 0x00;
		uint16_t tbus	 	= 0x00;
		uint16_t tdevice	= 0x14;
		uint16_t tfunction  = 0x07;
		uint16_t tdeviceid	= 0x00;
		uint16_t tvendorid	= 0x00;
		int32_t data 		= 0x00;
		bool result = false;

		/* Search SD Host available*/
		for(tbus = 0x00 ; tbus <= 0xFF ; tbus++)
		{
			for(tdevice = 0x00 ; tdevice <= 0x1F ; tdevice++)
			{
				for( tfunction = 0x00; tfunction <= 0x07 ; tfunction++)
				{
				    data = hd_read_pci((unsigned short)(tbus),(unsigned short)(tdevice),(unsigned short)(tfunction),(size_t)(SDHC_PCI_SUB_CLASS_CLASS_CODE),(unsigned short)(2));
					if(SD_DEVICE_CLASS == data)
					{
						if(!(node[total_pci].init_node(tbus,tdevice,tfunction)))
						{
						  printf("_pci_handler::init_node failed\n");
						  exit(1);
						}  
						total_pci++;
					}
				}
			}
		}

	}

	bool _pci_handler::get_pci_header(_pci_type0_header *header_ptr, uint32_t dev_number)
	{
		return(node[dev_number].get_header(header_ptr));
	}
	void _pci_handler::get_bus_device_function(uint32_t device , uint16_t *bus_number , uint16_t *device_number , uint16_t *function_number)
	{
		node[device].get_bus_device_function(bus_number , device_number , function_number);
	}

	uint8_t _pci_handler::get_total_dev()
	{
		return total_pci;
	}
	uint8_t _pci_handler::get_total_slot(uint32_t device)
	{
		return (PCI_TOTAL_SLOT(node[device].pci_header.slotinfo) + 1 );
	}
	_pci_handler::~_pci_handler()
	{
		/* Do nothing now */
	}

	bool _pci_handler::read_pci(_reg *preg)
	{
		bool result = false;
		uint32_t device 				= preg->pci_or_mmio_reg.device;
		node[device].reg.pci_or_mmio_reg.offset = preg->pci_or_mmio_reg.offset;
		node[device].reg.pci_or_mmio_reg.width  = preg->pci_or_mmio_reg.width;
		result = node[device].read_pci();
		
		preg->pci_or_mmio_reg.value = node[device].reg.pci_or_mmio_reg.value;
		return result;
	}

	bool _pci_handler::write_pci(_reg *preg)
	{
		bool result = false;
		uint32_t device 				= preg->pci_or_mmio_reg.device;
		node[device].reg.pci_or_mmio_reg.offset = preg->pci_or_mmio_reg.offset;
		node[device].reg.pci_or_mmio_reg.width  = preg->pci_or_mmio_reg.width;
		node[device].reg.pci_or_mmio_reg.value  = preg->pci_or_mmio_reg.value;
	
		return(node[device].write_pci());
	}

	size_t _pci_handler::get_bar(uint32_t dev_number, uint32_t slot)
	{
		size_t offset = 0x00;
		switch(slot)
		{
	       case 0:
	       offset += node[dev_number].pci_header.bar0;
	       break;
	       
	       case 1:
	       offset += node[dev_number].pci_header.bar1;	
	       break;
	       
	       case 2:
	       offset += node[dev_number].pci_header.bar2;	
	       break;
	       
	       case 3:
	       offset += node[dev_number].pci_header.bar3;	
	       break;
	       
	       case 4:
	       offset += node[dev_number].pci_header.bar4;	
	       break;
	       
	       case 5:
	       offset += node[dev_number].pci_header.bar5;	
	       break;
       }
	   return offset;
	}

	
#endif

