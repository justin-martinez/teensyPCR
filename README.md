# teensyPCR
Microcontroller code for PCR

PCR: 
	The polymerase chain reaction (PCR) is a technology in molecular biology used to amplify a single copy 
	or a few copies of a piece of DNA across several orders of magnitude, generating thousands to millions 
	of copies of a particular DNA sequence.The method relies on thermal cycling, consisting of cycles of 
	repeated heating and cooling of the reaction for DNA melting and enzymatic replication of the DNA. 
	Primers (short DNA fragments) containing sequences complementary to the target region along with a DNA 
	polymerase, which the method is named after, are key components to enable selective and repeated 
	amplification. As PCR progresses, the DNA generated is itself used as a template for replication, 
	setting in motion a chain reaction in which the DNA template is exponentially amplified. PCR can be 
	extensively modified to perform a wide array of genetic manipulations.
	
	Uses:
	These include DNA cloning for sequencing, DNA-based phylogeny, or functional analysis of genes; the 
	diagnosis of hereditary diseases; the identification of genetic fingerprints (used in forensic sciences 
	and paternity testing); and the detection and diagnosis of infectious diseases.
	 
	Steps:
	Initialization step: 
	(Only required for DNA polymerases that require
	heat activation by hot-start PCR): This step consists of heating
	the reaction to a temperature of 94–96 °C (or 98 °C if extremely thermostable
	polymerases are used), which is held for 1–9 minutes.
	
	Denaturation step: 
	This step is the first regular cycling event and
	consists of heating the reaction to 94–98 °C for 20–30 seconds. It causes
	DNA melting of the DNA template by disrupting the hydrogen bonds between
	complementary bases, yielding single-stranded DNA molecules.
	
	Annealing step: 
	The reaction temperature is lowered to 50–65 °C for 20–40
	seconds allowing annealing of the primers to the single-stranded DNA template.
	This temperature must be low enough to allow for hybridization of the primer to
	the strand, but high enough for the hybridization to be specific, i.e., the
	primer should only bind to a perfectly complementary part of the template. If
	the temperature is too low, the primer could bind imperfectly. If it is too high,
	the primer might not bind. Typically the annealing temperature is about 3–5 °C
	below the Tm of the primers used. Stable DNA–DNA hydrogen bonds are only formed
	when the primer sequence very closely matches the template sequence. The
	polymerase binds to the primer-template hybrid and begins DNA formation.
	
	Extension/elongation step: 
	The temperature at this step depends on the
	DNA polymerase used; Taq polymerase has its optimum activity temperature at
	75–80 °C, and commonly a temperature of 72 °C is used with this enzyme.
	At this step the DNA polymerase synthesizes a new DNA strand complementary
	to the DNA template strand by adding dNTPs that are complementary to the
	template in 5' to 3' direction, condensing the 5'-phosphate group of the dNTPs
	with the 3'-hydroxyl group at the end of the nascent (extending) DNA strand.
	The extension time depends both on the DNA polymerase used and on the length
	of the DNA fragment to amplify. As a rule-of-thumb, at its optimum temperature,
	the DNA polymerase polymerizes a thousand bases per minute. Under optimum
	conditions, i.e., if there are no limitations due to limiting substrates or
	reagents, at each extension step, the amount of DNA target is doubled, leading
	to exponential (geometric) amplification of the specific DNA fragment.
	
	Final elongation: 
	This single step is occasionally performed at a temperature of 70–74 °C (this 
	is the temperature needed for optimal activity for most polymerases used in PCR) 
	for 5–15 minutes after the last PCR cycle to ensure that any remaining 
	single-stranded DNA is fully extended.
	
	Final hold: 
	This step at 4–15 °C for an indefinite time may be employed for short-term storage 
	of the reaction.
