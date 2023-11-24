#include "card.h"
#include <env_time.h>

using namespace std;
coreutil::Logger CardLogger::mLogger("CARDLOG");

																 
void FRAME_CMD(_command* pcmd, uint16_t index,uint16_t cmd_type,bool data_present,bool index_check, bool crc_check, uint16_t response_type, uint32_t flag , uint32_t retries,uint32_t arg, uint32_t device, uint32_t slot)
{
  pcmd->cmd.bit.response_type		= response_type;
  pcmd->cmd.bit.crc_check			= crc_check;
  pcmd->cmd.bit.index_check			= index_check;
  pcmd->cmd.bit.data_present		= data_present;
  pcmd->cmd.bit.cmd_type			= cmd_type;
  pcmd->cmd.bit.index				= index;
  /*Reserved Bits*/
  pcmd->cmd.bit.rsvd1 = 0;
  pcmd->cmd.bit.rsvd2 = 0;
  
  
  
  pcmd->retries						= retries;
  pcmd->device_number				= device;
  pcmd->slot						= slot;
  pcmd->response_flag				= flag;
  pcmd->arg							= arg;
  pcmd->r1.reg_val					= 0x00;
  pcmd->r2.insert(0x0000000000000000,0x0000000000000000);
  pcmd->r3.reg_val					= 0x00;
  pcmd->r4.reg_val					= 0x00;
  pcmd->r5.reg_val					= 0x00;
  pcmd->r6.reg_val					= 0x00;
  pcmd->r7.reg_val					= 0x00;
  		
}

_cmd::_cmd()
{
  cmd_next		= 0;
  cmd_current	= 0;
}

_cmd::~_cmd()
{
}

bool _cmd::insert_cmd_queue(_command *cmd_info)
{
  p_cmd_ringbuffer[cmd_next] = cmd_info;
  if(10 == ++cmd_next)
	cmd_next = 0;

  return true;
}

bool _cmd::send_cmd(uint32_t device , uint32_t slot)
{
  uint8_t num_try = NUMBER_OF_WAIT_FOR_CMD_DAT_INHIBIT;
  uint32_t error  = 0x00;
  bool result = true;
  uint32_t retries = p_cmd_ringbuffer[cmd_current]->retries;
  if(retries == 0)
	  retries = 1;
  uint64_t rhs;
  uint64_t lhs;
  reg32 present_state;
  _reg sd_register;
  
  /*** Check CMD Inhibit ***/
  SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
	if(!rhost_reg(&sd_register))
	return false;
	present_state.reg_val = sd_register.sd_reg.value;
  do{
  		while(GET_CMD_INHIBIT_CMD_LINE(present_state))
  		{
  			env::SleepMs(2);
			SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
			if(!rhost_reg(&sd_register))
				return false;
			present_state.reg_val = sd_register.sd_reg.value;
  			num_try--;
			if(!(num_try))
  			break;
  		}

	  	#if 0
		if(response48_busy == p_cmd_ringbuffer[cmd_current]->cmd.bit.response_type)
		{
			num_try = NUMBER_OF_WAIT_FOR_CMD_DAT_INHIBIT;
			while(GET_CMD_INHIBIT_DAT_LINE(present_state))
			{
				printf("Data line Busy\n");
				env::SleepMs(2);
				SET_SD_REGISTER_READ(&sd_register,HOST_PRESENT_STATE,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
				if(!rhost_reg(&sd_register))
					return false;
				present_state.reg_val = sd_register.sd_reg.value;
				if(!(num_try--))
				break;
			}
			printf("Data line is not Busy\n");
			printf("CMD_w_busy : %X\n",p_cmd_ringbuffer[cmd_current]-> cmd.reg_val);
		}
		#endif
  		if(num_try)
  		{
			SET_SD_REGISTER_WRITE(&sd_register,HOST_ARGUMENT,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot,p_cmd_ringbuffer[cmd_current]-> arg);
  			whost_reg(&sd_register);
		
			SET_SD_REGISTER_WRITE(&sd_register,HOST_COMMAND,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot,p_cmd_ringbuffer[cmd_current]-> cmd.reg_val)	;	
  			whost_reg(&sd_register);
		
  			env::SleepMs(1);
			if(p_cmd_ringbuffer[cmd_current]->cmd.bit.index != CMD_19)
			{
				if(!cmd_complete(device , slot , &error))
				{
					p_cmd_ringbuffer[cmd_current]->error = error;
					result = false;
					//printf("CMD %d failed, cmd_cmplete Error : %X\n",p_cmd_ringbuffer[cmd_current]->cmd.bit.index,error);
					CARD_LOG_DEBUG() << "CMD failed : " << std::hex << p_cmd_ringbuffer[cmd_current]->cmd.bit.index;
					CARD_LOG_DEBUG() << "cmd_complete error : " << std::dec << error;
					//CARD_OST_LOG_DEBUG() << "Present State Register" << std::hex << prsnt_state_register.sd_reg.value << std::endl;

				}
			}
			if(result == true)
			//printf("CMD %d \n", p_cmd_ringbuffer[cmd_current]->cmd.bit.index);
			CARD_LOG_DEBUG() << "CMD" << std::dec << p_cmd_ringbuffer[cmd_current]->cmd.bit.index;
			
			uint64_t rhs_lower_32bit=0, rhs_upper_32bit=0;
			uint64_t lhs_lower_32bit=0, lhs_upper_32bit=0;
					
			switch(p_cmd_ringbuffer[cmd_current]->cmd.bit.response_type)
			{
            
			case noresponse:
				break;
			case response136:
				if(R2 == p_cmd_ringbuffer[cmd_current]->response_flag)
				{
					//read rhs lower 32 bit
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_0,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x04;
					if(!rhost_reg(&sd_register))
					return false;
					
					rhs_lower_32bit = sd_register.sd_reg.value;
								
					CARD_LOG_DEBUG() << " rhs_lower_32bit::  " << std::hex << rhs_lower_32bit<<std::endl;
					
												
					//read rhs uppper 32 bit
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_2,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x04;
					if(!rhost_reg(&sd_register))
						return false;
					rhs_upper_32bit = sd_register.sd_reg.value;
					
					CARD_LOG_DEBUG() << " rhs_upper_32bit::  " << std::hex << rhs_upper_32bit<<std::endl;
					
					
					rhs = (rhs_upper_32bit << 32) | rhs_lower_32bit;
					CARD_LOG_DEBUG() << " 64 bit rhs::  " << std::hex << rhs<<std::endl;
					
					
					// **************************************************
					
					//read lhs lower 32 bit
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_4,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x04;
					if(!rhost_reg(&sd_register))
					return false;
					
					lhs_lower_32bit = sd_register.sd_reg.value;
					CARD_LOG_DEBUG() << " lhs_lower_32bit  " << std::hex << lhs_lower_32bit<<std::endl;
								
					//read lhs upper 32 bit
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_6,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x04;
					if(!rhost_reg(&sd_register))
					return false;
					lhs_upper_32bit = sd_register.sd_reg.value;
					CARD_LOG_DEBUG() << " lhs_upper_32bit  " << std::hex << lhs_upper_32bit<<std::endl;
					
					lhs = (lhs_upper_32bit << 32) | lhs_lower_32bit;
					CARD_LOG_DEBUG() << " 64 bit lhs::  " << std::hex << lhs<<std::endl;
					
					
					
					p_cmd_ringbuffer[cmd_current]->r2.insert(rhs,lhs);
				}
				break;
			case response48:
			case response48_busy:
				if(R7 == p_cmd_ringbuffer[cmd_current]->response_flag)
				{
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_1,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x4;
					if(!rhost_reg(&sd_register))
					return false;
	  			    p_cmd_ringbuffer[cmd_current]->r7.reg_val = sd_register.sd_reg.value;
				}
            
				else if(R6 == p_cmd_ringbuffer[cmd_current]->response_flag)
				{
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_0,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x4;	
					if(!rhost_reg(&sd_register))
					return false;
					p_cmd_ringbuffer[cmd_current]->r6.reg_val = sd_register.sd_reg.value;
				}
            
				else if((R1 == p_cmd_ringbuffer[cmd_current]->response_flag) | (R1b == p_cmd_ringbuffer[cmd_current]->response_flag))
				{
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_0,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x4;	
					if(!rhost_reg(&sd_register))
					return false;
					p_cmd_ringbuffer[cmd_current]->r1.reg_val = sd_register.sd_reg.value;
				}
            
				else if(R3 == p_cmd_ringbuffer[cmd_current]->response_flag)
				{
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_0,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x4;	
					if(!rhost_reg(&sd_register))
					return false;
				
					p_cmd_ringbuffer[cmd_current]->r3.reg_val = sd_register.sd_reg.value;
				}
				else if(R4 == p_cmd_ringbuffer[cmd_current]->response_flag)
				{
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_0,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x4;	
					if(!rhost_reg(&sd_register))
					return false;
				
					p_cmd_ringbuffer[cmd_current]->r4.reg_val = sd_register.sd_reg.value;
				}
				
				else if(R5 == p_cmd_ringbuffer[cmd_current]->response_flag)
				{
					SET_SD_REGISTER_READ(&sd_register,HOST_RESPONSE_0,p_cmd_ringbuffer[cmd_current]->device_number,p_cmd_ringbuffer[cmd_current]->slot);
					sd_register.sd_reg.width = 0x4;	
					if(!rhost_reg(&sd_register))
					return false;
				
					p_cmd_ringbuffer[cmd_current]->r5.reg_val = sd_register.sd_reg.value;
				}
            
				break;
				
			}

  	}
  	else
  	{
  		result = false;
  		std::cout << "send_cmd failed GET_CMD_INHIBIT (DAT/CMD line) is not cleared "<< std::endl;
  	}
  }while(--retries);
 
  if(10 == ++cmd_current)
	cmd_current = 0;
  return result;
}
bool _cmd::cmd_complete(uint32_t device , uint32_t slot , uint32_t *_error)
{
return int_status(CMD_COMPLETE_INT ,device , slot , _error , 0x10);
}
bool _cmd::trns_complete(uint32_t device , uint32_t slot , uint32_t *_error, uint32_t retry)
{
return int_status(TRNS_COMPLETE_INT ,device , slot , _error ,retry );
}
bool _cmd::dma_complete(uint32_t device , uint32_t slot , uint32_t *_error)
{
return int_status(DMA_COMPLETE_INT ,device , slot , _error , 0x10);
}
bool _cmd::block_gap(uint32_t device , uint32_t slot , uint32_t *_error)
{
return int_status(BLOCK_GAP_INT ,device , slot , _error, 0x10);
}

bool _cmd::int_status(uint32_t type , uint32_t device , uint32_t slot , uint32_t *_error,uint32_t retry)
{
	uint32_t wait_loop = 0x10;
	bool done		  = false;
	bool result		  = false;
	reg32 int_status; 
	_reg sd_register;
	if(retry > wait_loop )
		wait_loop = retry;

	do{
		SET_SD_REGISTER_READ(&sd_register,HOST_INT_STATUS,device,slot);
		if(!rhost_reg(&sd_register))
		return false;
		int_status.reg_val = sd_register.sd_reg.value;
		
		if(GET_ERROR_INT(int_status))
		{
			*_error = GET_ERROR_INT_STATUS(int_status);
			done   = true;
			result = false;
			SET_SD_REGISTER_WRITE(&sd_register,HOST_INT_STATUS,device,slot,(int_status.reg_val));
			whost_reg(&sd_register);
		}
		else
		{
			if((type & (int_status.reg_val)))
			{
				SET_SD_REGISTER_WRITE(&sd_register,HOST_INT_STATUS,device,slot,(type & (int_status.reg_val)));
				whost_reg(&sd_register);
				done   = true;
				result = true;
			}
			else
			{   
				if(!wait_loop--)
				{
					//SET_SD_REGISTER_WRITE(&sd_register,HOST_INT_STATUS,device,slot,(type & (int_status.reg_val)));
					//whost_reg(&sd_register);
					done   = true;
					result = false;
					*_error = GET_ERROR_INT_STATUS(int_status);
				}
				env::SleepMs(100);
			}
		}
	}while(!done);
  //printf("INT Status : %x\n",sd_register.sd_reg.value);
  return result;
}
bool _card::cmd_complete(uint32_t *_error)
{
  return (cmd->cmd_complete(device , slot , _error));
}

bool _card::trns_complete(uint32_t *_error,uint32_t retry)
{
  return (cmd->trns_complete(device , slot , _error,retry));
}

bool _card::dma_complete(uint32_t *_error)
{
  return (cmd->dma_complete(device , slot , _error));  
}
bool _card::block_gap(uint32_t *_error)
{
return (cmd->block_gap(device , slot , _error));  
}
bool _card::send_command(_command *pcmd)
{
  if(!cmd->insert_cmd_queue(pcmd))
  {
	  return false;
  }
  if(!cmd->send_cmd(device , slot))
  {
	  return false;
  }
  return true;
}
bool _card::send_appcommand(_command *pcmd)
{
 uint32_t  arg = *rca << 16;
 uint32_t retries = 0x00;
 _command* pappcmd = new _command;
  FRAME_CMD(pappcmd, CMD_55, normal_cmd, false, true, true, response48 , R1 ,retries, arg, pcmd->device_number, pcmd->slot); 

  if(!send_command(pappcmd))
  {
	//printf("\n CMD55 Failed");	
	CARD_LOG_DEBUG() << "CMD55 Failed" << std::endl;	

	  return false;
  }
  if(pappcmd->r1.bit.app_cmd != true)
  {
	CARD_LOG_DEBUG() <<" APP_CMD not set" << std::endl ;
	  return false;
  }
  if(!send_command(pcmd))
  {
	  return false;
  }
  return true;
}

_card::_card(uint32_t tdevice, uint32_t tslot)
{
	cmd   						= new _cmd;
	ocr   						= new _r3;
	cid   						= new _r2;
	csd   						= new _r2;
	card_status 				= new _r1;
	sdio_ocr					= new _r4;
	rca 						= new uint16_t;
	dsr 						= new uint16_t;
	device 						= tdevice;
	slot 						= tslot;
	*rca 						= 0x00;
	*dsr						= 0x00;
	ocr->reg_val				= 0x00;
	card_status->reg_val		= 0x00;
	cid->insert(0x00 , 0x00);
	csd->insert(0x00 , 0x00);
}
_card::~_card()
{
	delete cmd;
	delete ocr;
	delete cid;
	delete card_status;
	delete rca;
	delete dsr;
}
void _card::reset_card_datastruct()
{
	*rca 						= 0x00;
	*dsr						= 0x00;
	ocr->reg_val				= 0x00;
	card_status->reg_val		= 0x00;
	cid->insert(0x00 , 0x00);
	csd->insert(0x00 , 0x00);
}
bool _card::go_idle(uint32_t *error)
{
  _command *ptrcmd = new _command;
  uint32_t retries = 0x00;
  uint32_t arg = 0x00;
  /*Send GO IDLE Command (CMD0)*/
  FRAME_CMD(ptrcmd, CMD_0, normal_cmd, false, false, false, noresponse ,0xFF ,retries, arg, device, slot);
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  return true;
}

bool _card::get_all_cid(_r2 *ptr_cid,uint32_t *error)
{
  /* Issue command ALL_SEND_CID (CMD2) to get the CID of the card */
  _command* ptrcmd 	= new _command; 
  uint32_t arg 		= 0x00;
  uint32_t retries 	= 0x00;
  FRAME_CMD(ptrcmd, CMD_2 , normal_cmd ,false, false, true, response136 , R2 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  cid->insert(ptrcmd->r2.rhs(),ptrcmd->r2.lhs());
  ptr_cid->insert(ptrcmd->r2.rhs(),ptrcmd->r2.lhs());
  return true;
}

bool _card::get_rca(uint16_t* ptr_rca,uint32_t *error)
{
  /* Issue command ALL_SEND_CID (CMD3) to get the CID of the card */
  _command* ptrcmd 	= new _command;
  uint32_t arg 		= 0x00;
  uint32_t retries 	= 0x00;
  FRAME_CMD(ptrcmd, CMD_3 , normal_cmd ,false, true, true, response48 , R6 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  *rca = ptrcmd->r6.bit.rca;
  *ptr_rca = *rca;  
  return true;
}

bool _card::get_rca_r6(uint16_t* ptr_rca,uint32_t *error,_r6 *rca_status)
{
  /* Issue command ALL_SEND_CID (CMD3) to get the CID of the card */
  _command* ptrcmd 	= new _command;
  uint32_t arg 		= 0x00;
  uint32_t retries 	= 0x00;
  FRAME_CMD(ptrcmd, CMD_3 , normal_cmd ,false, true, true, response48 , R6 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  *rca = ptrcmd->r6.bit.rca;
  *ptr_rca = *rca;  
  rca_status->reg_val = ptrcmd->r6.reg_val;
  return true;
}

bool _card::get_card_status(_r1 *ptr_cardstatus,uint32_t *error)
{
  /* Issue command SEND_STSTUS (CMD_13) to get the CID of the card */
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  uint32_t arg = (*rca << 16);
  FRAME_CMD(ptrcmd, CMD_13, normal_cmd, false, true, true, response48 , R1 ,retries, arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  card_status->reg_val = ptrcmd->r1.reg_val;
  ptr_cardstatus->reg_val = card_status->reg_val;
  return true;
}

bool _card::send_if_cond(uint8_t vhs,_r7* ptr_r7,uint32_t *error)
{
  /* Send Interface condition command (CMD 8) */
  uint32_t arg = vhs;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  arg = ((arg << 8) | 0x000000AA);
  FRAME_CMD(ptrcmd, CMD_8, normal_cmd, false, true, true, response48 , R7 ,retries, arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  ptr_r7->reg_val = ptrcmd->r7.reg_val;

  if((ptr_r7->bit.check_pattern) == 0xAA) 
//	printf("card_init: Pattern Matched in CMD8 response, CMD8 Pattern 0xAA, response pattern 0x%x\n", ptr_r7->bit.check_pattern);
	 CARD_LOG_DEBUG() << "card_init: Pattern Matched in CMD8 response, CMD8 Pattern 0xAA, response pattern 0x%x\n" << std::hex << ptr_r7->bit.check_pattern << std::endl;
 else
 {
//	printf("card_init: Failed to Matched pattern in CMD8 response, CMD8 Pattern 0xAA, response pattern 0x%x\n", ptr_r7->bit.check_pattern);
	 CARD_LOG_DEBUG() << "card_init: Failed to Matched pattern in CMD8 response, CMD8 Pattern 0xAA, response pattern 0x%x\n"<< std::hex << ptr_r7->bit.check_pattern<< std::endl;
	return false;
 }

  return true;
}
bool _card::get_ocr(_r3* ptr_ocr,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  
  FRAME_CMD(ptrcmd, ACMD_41 , normal_cmd ,false, false, false, response48 , R3 ,retries , arg, device, slot); 
  if(!send_appcommand(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  ocr->reg_val     = ptrcmd->r3.reg_val;	
  ptr_ocr->reg_val = ocr->reg_val;	  
  return true;
}

bool _card::get_sdio_ocr(_r4* ptr_ocr,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  
  FRAME_CMD(ptrcmd, CMD_5 , normal_cmd ,false, false, false, response48 , R4 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  sdio_ocr->reg_val	= ptrcmd->r4.reg_val;	
  ptr_ocr->reg_val 	= sdio_ocr->reg_val;	  
  return true;
}

bool _card::send_op_cond(bool s18r,bool hcs,_r3* ptr_ocr,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  uint16_t MaxTry = 20;
 
  do{
	arg = (((ocr->reg_val) & (0xFFFF << 8)) | (hcs << 30) | (s18r << 24));
	FRAME_CMD(ptrcmd, ACMD_41 , normal_cmd ,false ,false,false, response48 , R3 ,retries , arg, device, slot); 
	if(!send_appcommand(ptrcmd))
	{
		*error = ptrcmd-> error;
		return false;
	}
  *error = 0x00;
  	env::SleepMs(10);
  }while((ptrcmd->r3.bit.busy == 0) && (MaxTry--)); 
   if(!MaxTry)
	return false;
   ptr_ocr->reg_val = ptrcmd->r3.reg_val;	
	return true;
}

bool _card::sdio_send_op_cond(bool s18r,_r4* ptr_ocr,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  uint16_t MaxTry = 20;
 
  do{
	arg = (((sdio_ocr->reg_val) & (0xFFFF << 8)) | (s18r << 24));
	FRAME_CMD(ptrcmd, CMD_5 , normal_cmd ,false ,false,false, response48 , R4 ,retries , arg, device, slot); 
	if(!send_command(ptrcmd))
	{
		*error = ptrcmd-> error;
		return false;
	}
  *error = 0x00;
  	env::SleepMs(10);
  }while((ptrcmd->r4.bit.c == 0) && (MaxTry--));  
    ptr_ocr->reg_val = ptrcmd->r4.reg_val;	
  return true;
}



bool _card::get_cid(_r2* ptr_cid,uint32_t *error)
{
  /* Issue command SEND_CID (CMD10) to get the CID of the card */
  _command* ptrcmd = new _command; 
  uint32_t arg = (*rca << 16);
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_10 , normal_cmd ,false, false, true, response136 , R2 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  cid->insert(ptrcmd->r2.rhs(),ptrcmd->r2.lhs());
  ptr_cid->insert(ptrcmd->r2.rhs(),ptrcmd->r2.lhs());
  return true;
}

bool _card::get_csd(_r2* ptr_csd,uint32_t *error)
{
  /* Issue command SEND_CID (CMD10) to get the CID of the card */
  _command* ptrcmd = new _command; 
  uint32_t arg = (*rca << 16);
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_9 , normal_cmd ,false, false, true, response136 , R2 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  csd->insert(ptrcmd->r2.rhs(),ptrcmd->r2.lhs());
  ptr_csd->insert(ptrcmd->r2.rhs(),ptrcmd->r2.lhs());
  return true;
}

bool _card::get_scr(_r1 *status,uint32_t *error)
{
 /* Issue SEND_SCR_CMD (ACMD51) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= 0x00;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, ACMD_51 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_appcommand(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::select_card(_r1 *status,uint32_t *error)
{
 /* Issue SELECT/DESELECT CARD (CMD7) */
  _command* ptrcmd = new _command; 
  uint32_t arg = *rca;
  arg = (arg << 16);
  //std::cout << " CMD 7 arg : " << std::hex << arg << std::endl;
  uint32_t retries = 0x00;

  //printf("rca value %x\n", arg);

 //FRAME_CMD(ptrcmd, CMD_7 , normal_cmd ,false, true, true, response48_busy , R1b ,retries , arg, device, slot); 
  FRAME_CMD(ptrcmd, CMD_7 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::deselect_card(_r1 *status,uint32_t *error)
{
 /* Issue SELECT/DESELECT CARD (CMD7) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg 		= 0x00;
  uint32_t retries 	= 0x00;
  FRAME_CMD(ptrcmd, CMD_7 , normal_cmd ,false, true, true, noresponse , 0xFF ,retries , arg, device, slot);   
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::set_buswidth(uint8_t width,_r1 *status,uint32_t *error)
{
 /* Issue SET_BUS_WIDTH CMD (ACMD6) */
  _command* ptrcmd = new _command; 
  uint32_t arg = (0x00 | width);
  uint32_t retries = 0x00;
  //FRAME_CMD(_command* pcmd, uint16_t index,uint16_t cmd_type,bool data_present,bool index_check, bool crc_check, uint16_t response_type, uint32_t flag , uint32_t retries,uint32_t arg, uint32_t device, uint32_t slot)
  FRAME_CMD(ptrcmd, ACMD_6 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_appcommand(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::set_blocklength(uint32_t blocklength,_r1 *status,uint32_t *error)
{
 /* Issue SET_BLOCK_LENGTH CMD (CMD16) */
  _command* ptrcmd = new _command; 
  uint32_t arg = blocklength;
  uint32_t retries = 0x00;
  //printf("Card Block Length : %X\n", arg);
  FRAME_CMD(ptrcmd, CMD_16 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::read_singleblock(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue READ SINGLE BLOCK CMD (CMD17) */
  _command* ptrcmd = new _command; 
  uint32_t arg = address;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_17 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::read_multiblock(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue READ MULTI BLOCK CMD (CMD18) */
  _command* ptrcmd = new _command; 
  uint32_t arg = address;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_18 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::write_singleblock(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue WRITE SINGLE BLOCK CMD (CMD24) */
  _command* ptrcmd = new _command; 
  uint32_t arg = address;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_24 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::write_multiblock(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue WRITE MULTI BLOCK CMD (CMD25) */
  _command* ptrcmd = new _command; 
  uint32_t arg = address;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_25 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::write_multiblock_error(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue WRITE MULTI BLOCK CMD (CMD25) */
  _command* ptrcmd = new _command; 
  uint32_t arg = address;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, 0x3F , normal_cmd ,false, false, false, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}


bool _card::set_blockcount(uint32_t blockcount,_r1 *status,uint32_t *error)
{
 /* Issue SET_BLOCK_COUNT CMD (CMD23) */
  _command* ptrcmd = new _command; 
  uint32_t arg = blockcount;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_23 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::go_inactive(uint32_t *error)
{
 /* Issue GO_INACTIVE CMD (CMD15) */
  _command* ptrcmd = new _command; 
  uint32_t arg = *rca;
  arg = arg << 16;
  uint32_t retries = 0x00;
  FRAME_CMD(ptrcmd, CMD_15 , normal_cmd ,false, false, true, noresponse , 0xFF ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  return true;
}

bool _card::erase_start_add(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue ERASE_WR_BLK_START CMD (CMD32) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg 		= address;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_32 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::erase_end_add(uint32_t address,_r1 *status,uint32_t *error)
{
 /* Issue ERASE_WR_BLK_START CMD (CMD33) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg 		= address;
  uint32_t retries	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_33 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::erase(_r1 *status,uint32_t *error)
{
 /* Issue ERASE_WR_BLK_START CMD (CMD38) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= 0x00;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_38 , normal_cmd ,false, true, true, response48_busy , R1b ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::lock_unlock(_r1 *status,uint32_t *error)
{
 /* Issue LOCK_UNLOCK CMD (CMD42) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= 0x00;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_42 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::switch_function(_r1 *status, uint32_t argument,uint32_t *error)
{
 /* Issue SWITCH_FUNCTION_CMD (CMD6) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= argument;
  uint32_t retries 	= 0x00;
  
  //FRAME_CMD(_command* pcmd, uint16_t index,uint16_t cmd_type,bool data_present,bool index_check, bool crc_check, uint16_t response_type, uint32_t flag , uint32_t retries,uint32_t arg, uint32_t device, uint32_t slot)
  
  FRAME_CMD(ptrcmd, CMD_6 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}
bool _card::voltage_switch(_r1 *status,uint32_t *error)
{
 /* Issue VOLTAGE_SWITCH_CMD (CMD11) */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= 0x00;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_11 , normal_cmd ,false, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
	  return true;
}

bool _card::get_sd_status(_r1 *status,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  
  FRAME_CMD(ptrcmd, ACMD_13 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_appcommand(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;	  
  return true;
}
bool _card::stop_transmission(_r1 *status,uint32_t *error)
{
 /* Issue CMD12 */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= 0x00;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_12 , abort_cmd ,false, true, true, response48_busy , R1b ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
  return true;
}

bool _card::send_tuning_block(_r1 *status,uint32_t *error)
{
 /* Issue CMD19 */
  _command* ptrcmd 	= new _command; 
  uint32_t arg     	= 0x00;
  uint32_t retries 	= 0x00;
  
  FRAME_CMD(ptrcmd, CMD_19 ,  normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;
  return true;
}

bool _card::send_num_wr_blocks(_r1 *status,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  
  FRAME_CMD(ptrcmd, ACMD_22 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_appcommand(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;	  
  return true;
}

bool _card::program_csd(_r1 *status,uint32_t *error)
{
  uint32_t arg = 0x00;
  _command* ptrcmd = new _command;
  uint32_t retries = 0x00;
  
  FRAME_CMD(ptrcmd, CMD_27 , normal_cmd ,true, true, true, response48 , R1 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r1.reg_val;	  
  return true;
}



// SDIO Read/Write commands
bool _card::send_sdio_command(uint32_t arg, uint32_t cmd, _r5 *status,uint32_t *error)
{
  _command* ptrcmd 	= new _command; 
  uint32_t retries 	= 0x00; 

 //printf("CMD 52 Argument = 0x%X",arg);
  CARD_LOG_DEBUG() <<"CMD 52 Argument =" << std::hex << arg;
  FRAME_CMD(ptrcmd, cmd ,  normal_cmd ,false, false, true, response48 , R5 ,retries , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r5.reg_val;
  return true;

}


bool _card::direct_io_read(uint32_t function,uint32_t reg_offset, _r5 *status,uint32_t *error)
{
 /* Issue CMD52 */
  uint32_t arg     	= 0x00;
  arg = (arg | (reg_offset << 9) | (function << 28) );
  return (send_sdio_command(arg, CMD_52, status , error));
}

bool _card::direct_io_write(bool raw , uint32_t function,uint32_t reg_offset, uint32_t data , _r5 *status,uint32_t *error)
{
 /* Issue CMD52 */
  uint32_t arg     	= 0x00;
  arg = (arg | (1 << 31) | ((raw == true ? 1 : 0) << 27) | (reg_offset << 9) | (function << 28) | data );
  return (send_sdio_command(arg, CMD_52, status , error));
}


bool _card::extended_io_read(uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address,bool block_mode, uint32_t count, _r5 *status,uint32_t *error)
{
 /* Issue CMD53 */
  uint32_t arg     	= 0x00;
  _command* ptrcmd 	= new _command; 
  
  arg = (arg | (count & 0x1FF) | ((reg_offset & 0x1FFFF) << 9) | ((fixe_or_inc_address == true ? 1 : 0) << 26) | ((block_mode == true ? 1 : 0) << 27) | ( (function & 0x07) << 28));

  FRAME_CMD(ptrcmd, CMD_53 ,  normal_cmd ,true, true, true, response48 , R5 ,0x00 , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r5.reg_val;
  return true;
 }

bool _card::extended_io_write(uint32_t function, uint32_t reg_offset, bool fixe_or_inc_address,bool block_mode, uint32_t count, _r5 *status,uint32_t *error)
{
 /* Issue CMD53 */
  uint32_t arg     	= 0x00;
  _command* ptrcmd 	= new _command; 
  
  arg = (arg | (count & 0x1FF) | ((reg_offset & 0x1FFFF) << 9) | ((fixe_or_inc_address == true ? 1 : 0) << 26) | ((block_mode == true ? 1 : 0) << 27) | ( (function & 0x07) << 28) | (1 << 31) );

  FRAME_CMD(ptrcmd, CMD_53 ,  normal_cmd ,true, true, true, response48 , R5 ,0x00 , arg, device, slot); 
  if(!send_command(ptrcmd))
  {
	*error = ptrcmd-> error;
    return false;
  }
  *error = 0x00;
  status->reg_val = ptrcmd->r5.reg_val;
  return true;
}
