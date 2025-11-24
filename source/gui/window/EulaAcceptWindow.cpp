/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "EulaAcceptWindow.hpp"

#include "gui/lookandfeel/Constants.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "ravennakit/core/support.hpp"

#include <juce_cryptography/juce_cryptography.h>

namespace
{

// Note: changing the text will trigger the accept window for users who accepted a previous version (because the hash will not be the same).
auto eulaTxt =
    R"(END-USER LICENSE AGREEMENT (EVALUATION / DEMO SOFTWARE)

Last updated: November 23, 2025

This End-User License Agreement (“Agreement”) is a legal agreement between you (“You” or “User”) and [COMPANY NAME], a company organized under the laws of [COUNTRY] (“Company”), regarding your use of the [APP NAME] demo application (the “Software”).

By installing, copying, or otherwise using the Software, you agree to be bound by the terms of this Agreement. If you do not agree to these terms, do not install or use the Software.

1. LICENSE GRANT (EVALUATION ONLY)

1.1 Limited License.
Subject to your full and ongoing compliance with this Agreement, Company grants you a personal, non-exclusive, non-transferable, revocable, limited license to install and use the Software solely for your internal evaluation and testing purposes.

1.2 No Production Use.
You may not use the Software in any production environment, commercial service, or for providing services to third parties. The Software is intended only for evaluation, testing, and demonstration.

1.3 No SDK License.
The Software may incorporate or be built upon Company’s proprietary software development kit [SDK NAME] (the “SDK”). This Agreement does not grant you any license or rights to use the SDK independently of the Software, to integrate it into your own products, or to redistribute it in any form. Any such rights require a separate written agreement with Company.

2. RESTRICTIONS

You shall not, and shall not permit any third party to:

2.1 Copy, modify, adapt, translate, or create derivative works based on the Software or SDK, except as expressly allowed by this Agreement.

2.2 Reverse engineer, decompile, disassemble, or otherwise attempt to derive the source code, underlying ideas, algorithms, or file formats of the Software or SDK, except to the limited extent permitted by applicable mandatory law.

2.3 Remove, alter, or obscure any proprietary notices, labels, trademarks, or copyright notices on or in the Software.

2.4 Sell, sublicense, distribute, rent, lease, lend, host, or otherwise make the Software or SDK available to any third party.

2.5 Use the Software for any unlawful purpose or in violation of any applicable law or regulation.

2.6 Use the Software in safety-critical systems or environments where failure of the Software could reasonably be expected to result in personal injury, death, or severe physical or environmental damage.

3. INTELLECTUAL PROPERTY

3.1 Ownership.
The Software, the SDK, and all related documentation and materials are protected by copyright and other intellectual property laws and treaties. Company and its licensors own and shall retain all right, title, and interest in and to the Software, SDK, and any copies thereof, including all intellectual property rights.

3.2 Feedback.
If you provide feedback, suggestions, or comments regarding the Software or SDK (“Feedback”), you grant Company a perpetual, irrevocable, worldwide, royalty-free license to use, reproduce, modify, and otherwise exploit such Feedback without restriction, including for product improvement and development.

4. TERM AND TERMINATION

4.1 Term.
This Agreement is effective from the date you first install or use the Software and continues until terminated as set forth herein.

4.2 Automatic Termination.
This Agreement and your license rights will automatically terminate without notice from Company if you breach any provision of this Agreement.

4.3 Termination by Company.
Company may terminate this Agreement, and your license to use the Software, at any time for any reason or no reason, with or without notice.

4.4 Effect of Termination.
Upon termination of this Agreement for any reason, you must immediately stop using the Software and destroy all copies of the Software in your possession or control. Sections 2–9 of this Agreement shall survive termination.

5. DISCLAIMER OF WARRANTIES

THE SOFTWARE (INCLUDING THE SDK AS EMBEDDED THEREIN) IS PROVIDED “AS IS” AND “AS AVAILABLE”, WITH ALL FAULTS AND WITHOUT WARRANTY OF ANY KIND.

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, COMPANY AND ITS LICENSORS EXPRESSLY DISCLAIM ALL WARRANTIES, WHETHER EXPRESS, IMPLIED, STATUTORY OR OTHERWISE, INCLUDING BUT NOT LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE, AND NON-INFRINGEMENT.

COMPANY DOES NOT WARRANT THAT THE SOFTWARE WILL BE ERROR-FREE, UNINTERRUPTED, SECURE, OR THAT DEFECTS WILL BE CORRECTED, OR THAT THE SOFTWARE WILL BE COMPATIBLE WITH YOUR HARDWARE, SYSTEMS, OR OTHER SOFTWARE.

6. LIMITATION OF LIABILITY

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW:

6.1 IN NO EVENT SHALL COMPANY OR ITS LICENSORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL, CONSEQUENTIAL, SPECIAL, EXEMPLARY, OR PUNITIVE DAMAGES, OR ANY LOSS OF PROFITS, REVENUE, DATA, OR BUSINESS, ARISING OUT OF OR IN CONNECTION WITH THIS AGREEMENT OR YOUR USE OF (OR INABILITY TO USE) THE SOFTWARE, EVEN IF COMPANY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

6.2 COMPANY’S TOTAL AGGREGATE LIABILITY ARISING OUT OF OR RELATING TO THIS AGREEMENT OR THE SOFTWARE SHALL NOT EXCEED THE AMOUNT PAID BY YOU (IF ANY) FOR THE SOFTWARE, OR [€100], WHICHEVER IS GREATER.

Some jurisdictions do not allow the exclusion or limitation of certain damages or implied warranties, so the above limitations may not apply to you to that extent.

7. EXPORT CONTROL

You agree to comply with all applicable export and import laws and regulations. You represent that you are not located in a country or on a list where receiving the Software is prohibited or restricted under applicable export control laws.

8. GOVERNING LAW AND JURISDICTION

This Agreement shall be governed by and construed in accordance with the laws of [COUNTRY / STATE], without regard to its conflict of laws principles.

You and Company agree that any dispute arising out of or in connection with this Agreement shall be submitted to the exclusive jurisdiction of the courts located in [CITY, COUNTRY], and you hereby consent to personal jurisdiction and venue in such courts.

9. GENERAL

9.1 Entire Agreement.
This Agreement constitutes the entire agreement between you and Company regarding the Software and supersedes all prior or contemporaneous agreements, understandings, and communications, whether written or oral, relating to its subject matter.

9.2 Amendments.
Company may update or modify this Agreement from time to time. Your continued use of the Software after any such modification constitutes your acceptance of the revised Agreement.

9.3 Severability.
If any provision of this Agreement is held to be invalid or unenforceable, that provision shall be enforced to the maximum extent permissible and the remaining provisions shall remain in full force and effect.

9.4 No Assignment.
You may not assign or transfer this Agreement or any rights or obligations hereunder without the prior written consent of Company. Any attempted assignment in violation of this section shall be null and void.

BY CLICKING “I AGREE”, INSTALLING, OR USING THE SOFTWARE, YOU ACKNOWLEDGE THAT YOU HAVE READ THIS AGREEMENT, UNDERSTAND IT, AND AGREE TO BE BOUND BY ITS TERMS.
)";

struct EulaAcceptWindowComponent : juce::Component
{
    EulaAcceptWindowComponent()
    {
        title.setFont (juce::FontOptions (20));
        title.setText ("End User License Agreement", juce::dontSendNotification);
        title.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (title);

        subTitle.setFont (juce::FontOptions (16));
        subTitle.setText ("Please read and accept the following agreement to continue", juce::dontSendNotification);
        subTitle.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (subTitle);

        editor.setReadOnly (true);
        editor.setMultiLine (true);
        editor.setText (juce::CharPointer_UTF8 (eulaTxt), false);
        addAndMakeVisible (editor);

        acceptButton.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::buttonSuccess);
        acceptButton.setButtonText ("Accept");
        addAndMakeVisible (acceptButton);

        declineButton.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
        declineButton.setButtonText ("Decline");
        addAndMakeVisible (declineButton);
    }

    void resized() override
    {
        constexpr auto margin = 20;
        auto b = getLocalBounds().reduced (margin);

        title.setBounds (b.removeFromTop (30));
        subTitle.setBounds (b.removeFromTop (20));

        b.removeFromTop (margin);

        constexpr int buttonWidth = 100;
        constexpr int buttonHeight = 30;
        constexpr int buttonGap = 20;
        auto row = b.removeFromBottom (buttonHeight).withSizeKeepingCentre (buttonWidth * 2 + buttonGap, buttonHeight);
        acceptButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (buttonGap);
        declineButton.setBounds (row.removeFromLeft (buttonWidth));

        b.removeFromBottom (margin);

        editor.setBounds (b);
    }

    juce::Label title;
    juce::Label subTitle;
    juce::TextEditor editor;
    juce::TextButton acceptButton;
    juce::TextButton declineButton;
};

} // namespace

EulaAcceptWindow::EulaAcceptWindow (const juce::String& name) :
    ResizableWindow (name, rav::get_global_instance_of_type<ThisLookAndFeel>().findColour (backgroundColourId), true)
{
    setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());
    setUsingNativeTitleBar (true);

    auto comp = std::make_unique<EulaAcceptWindowComponent>();
    comp->acceptButton.onClick = [this] {
        onEulaAccepted (getEulaHash());
        exitModalState();
    };
    comp->declineButton.onClick = [this] {
        onEulaDeclined();
        exitModalState();
    };
    setContentOwned (comp.release(), true);

    setResizable (true, true);
    centreWithSize (getWidth(), getHeight());

    enterModalState();
}

EulaAcceptWindow::~EulaAcceptWindow()
{
    setLookAndFeel (nullptr);
}

juce::String EulaAcceptWindow::getEulaHash()
{
    static const auto hash = juce::MD5 (juce::CharPointer_UTF8 (eulaTxt)).toHexString();
    jassert (hash.isNotEmpty());
    return hash;
}
