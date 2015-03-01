
//list of packets we need to encode on the way out:
//E(OP_GuildMemberLevelUpdate)
//E(OP_SendCharInfo)
//E(OP_ZoneServerInfo)
E(OP_SendAATable)
//E(OP_PlayerProfile)
//E(OP_NewZone)
//E(OP_SpawnDoor)
//E(OP_SendZonepoints)
//E(OP_GuildMemberList)
//E(OP_Illusion)
//E(OP_ManaChange)
//E(OP_ClientUpdate)
//E(OP_LeadershipExpUpdate)
E(OP_ExpansionInfo)
E(OP_LogServer)
//E(OP_Buff)
//E(OP_Action)
//E(OP_Consider)
//E(OP_ShopPlayerSell)
//E(OP_OpenNewTasksWindow)
E(OP_BazaarSearch)
E(OP_Trader)
E(OP_TraderBuy)
//E(OP_LootItem)
E(OP_TributeItem)
E(OP_SomeItemPacketMaybe)
//E(OP_ReadBook)
E(OP_Stun)
E(OP_ZonePlayerToBind)
E(OP_AdventureMerchantSell)
E(OP_RaidUpdate)
E(OP_RaidJoin)
E(OP_VetRewardsAvaliable)
//E(OP_InspectRequest)
//E(OP_GroupInvite)
//E(OP_GroupFollow)
//E(OP_GroupFollow2)
//E(OP_GroupUpdate)
//E(OP_GroupCancelInvite)
//E(OP_WhoAllResponse)
E(OP_Track)
//E(OP_ShopPlayerBuy)
E(OP_PetBuffWindow)
//E(OP_OnLevelMessage)
E(OP_Barter)
//E(OP_ApplyPoison)
//E(OP_ChannelMessage)
//E(OP_GuildsList)
E(OP_DzExpeditionEndsWarning)
E(OP_DzExpeditionInfo)
E(OP_DzCompass)
E(OP_DzMemberList)
E(OP_DzExpeditionList)
E(OP_DzLeaderStatus)
E(OP_DzJoinExpeditionConfirm)
//E(OP_TargetBuffs)
//E(OP_BuffCreate)
//E(OP_SpawnAppearance)
E(OP_RespondAA)
E(OP_DisciplineUpdate)
E(OP_AltCurrencySell)
//E(OP_AltCurrency)
//E(OP_WearChange)
//E(OP_MercenaryDataResponse)
//E(OP_MercenaryDataUpdate)
//list of packets we need to decode on the way in:
//D(OP_CharacterCreate)
//D(OP_ItemLinkClick)
//D(OP_ConsiderCorpse)
//D(OP_Consider)
//D(OP_ClientUpdate)
//D(OP_WhoAllRequest)
//D(OP_Buff)
//D(OP_ShopPlayerSell)
//D(OP_CastSpell)
//D(OP_Save)
//D(OP_GroupInvite)
//D(OP_GroupInvite2)
//D(OP_GroupFollow)
//D(OP_GroupFollow2)
//D(OP_GroupDisband)
//D(OP_GroupCancelInvite)
//D(OP_FindPersonRequest)
D(OP_TraderBuy)
//D(OP_LootItem)
D(OP_TributeItem)
//D(OP_ReadBook)
//D(OP_AugmentInfo)
//D(OP_FaceChange)
D(OP_AdventureMerchantSell)
//D(OP_TradeSkillCombine)
//D(OP_RaidInvite)
//D(OP_InspectRequest)
//D(OP_WearChange)
//D(OP_ShopPlayerBuy)
D(OP_BazaarSearch)
//D(OP_LoadSpellSet)
//D(OP_ApplyPoison)
//D(OP_EnvDamage)
//D(OP_ChannelMessage)
//D(OP_AugmentItem)
//D(OP_PetCommands)
//D(OP_BuffRemoveRequest)
D(OP_AltCurrencySellSelection)
D(OP_AltCurrencySell)
#undef E
#undef D
